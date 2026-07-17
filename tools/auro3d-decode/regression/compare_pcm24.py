#!/usr/bin/env python3
"""Compare two PCM24 WAVs for AuroCX native-oracle validation.

Reports first mismatched sample and per-channel max abs / RMS error.
Exit 0 on exact equality; 1 on mismatch; 2 on I/O / format errors.
"""
from __future__ import annotations

import argparse
import math
import sys
import wave
from pathlib import Path


def pcm24(data: bytes, offset: int) -> int:
    value = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16)
    return value - 0x1000000 if value & 0x800000 else value


def open_pcm24(path: Path):
    wav = wave.open(str(path), "rb")
    if wav.getsampwidth() != 3:
        wav.close()
        raise RuntimeError(f"{path}: expected PCM24 (sample_width=3)")
    return wav


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ours", type=Path, help="decoded WAV from auro3d-decode")
    parser.add_argument("oracle", type=Path, help="native-oracle PCM24 WAV")
    parser.add_argument(
        "--max-frames",
        type=int,
        default=0,
        help="compare only the first N frames (0 = all)",
    )
    parser.add_argument(
        "--report-channels",
        type=int,
        default=8,
        help="print per-channel stats for the first N channels",
    )
    args = parser.parse_args()

    try:
        ours = open_pcm24(args.ours)
        oracle = open_pcm24(args.oracle)
    except Exception as exc:  # noqa: BLE001 — CLI surface
        print(f"error: {exc}", file=sys.stderr)
        return 2

    with ours, oracle:
        if (
            ours.getnchannels() != oracle.getnchannels()
            or ours.getframerate() != oracle.getframerate()
        ):
            print(
                "error: geometry mismatch "
                f"(ours ch={ours.getnchannels()} sr={ours.getframerate()}; "
                f"oracle ch={oracle.getnchannels()} sr={oracle.getframerate()})",
                file=sys.stderr,
            )
            return 2
        channels = ours.getnchannels()
        frames = min(ours.getnframes(), oracle.getnframes())
        if args.max_frames > 0:
            frames = min(frames, args.max_frames)
        if ours.getnframes() != oracle.getnframes():
            print(
                f"warning: frame count differs "
                f"(ours={ours.getnframes()} oracle={oracle.getnframes()}); "
                f"comparing {frames} frames"
            )

        frame_bytes = channels * 3
        max_abs = [0] * channels
        sum_sq = [0.0] * channels
        sum_x = [0.0] * channels
        sum_y = [0.0] * channels
        sum_xx = [0.0] * channels
        sum_yy = [0.0] * channels
        sum_xy = [0.0] * channels
        first = None
        compared = 0
        chunk = 4096
        remaining = frames
        while remaining > 0:
            take = min(chunk, remaining)
            a = ours.readframes(take)
            b = oracle.readframes(take)
            if len(a) != take * frame_bytes or len(b) != take * frame_bytes:
                print("error: short read", file=sys.stderr)
                return 2
            for frame in range(take):
                base = frame * frame_bytes
                for ch in range(channels):
                    x = pcm24(a, base + ch * 3)
                    y = pcm24(b, base + ch * 3)
                    xf = float(x)
                    yf = float(y)
                    diff = abs(x - y)
                    if diff > max_abs[ch]:
                        max_abs[ch] = diff
                    sum_sq[ch] += float(diff) * float(diff)
                    sum_x[ch] += xf
                    sum_y[ch] += yf
                    sum_xx[ch] += xf * xf
                    sum_yy[ch] += yf * yf
                    sum_xy[ch] += xf * yf
                    if diff and first is None:
                        first = (compared + frame, ch, x, y, x - y)
            compared += take
            remaining -= take

    exact = first is None
    print(f"frames_compared={compared} channels={channels} exact={int(exact)}")
    if first is not None:
        f, ch, x, y, d = first
        print(f"first_mismatch frame={f} channel={ch} ours={x} oracle={y} delta={d}")
    limit = min(channels, max(0, args.report_channels))
    for ch in range(limit):
        rms = math.sqrt(sum_sq[ch] / compared) if compared else 0.0
        corr = float("nan")
        if compared:
            n = float(compared)
            num = n * sum_xy[ch] - sum_x[ch] * sum_y[ch]
            den_x = n * sum_xx[ch] - sum_x[ch] * sum_x[ch]
            den_y = n * sum_yy[ch] - sum_y[ch] * sum_y[ch]
            den = math.sqrt(max(den_x, 0.0) * max(den_y, 0.0))
            if den > 0.0:
                corr = num / den
        print(
            f"channel[{ch}] max_abs={max_abs[ch]} rms_err={rms:.3f} corr={corr:.6f}"
        )
    return 0 if exact else 1


if __name__ == "__main__":
    raise SystemExit(main())
