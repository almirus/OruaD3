#!/usr/bin/env python3
import math
import subprocess
import sys
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / "bin" / "Release" / "auro3d-decode.exe"
TEST_FILES = ROOT / "test_files"


def run_decoder(input_name, output_name, output_bits=24):
    output = TEST_FILES / output_name
    cmd = [
        str(EXE),
        "--input",
        str(TEST_FILES / input_name),
        "--output",
        str(output),
        "--output-bits",
        str(output_bits),
        "-v",
    ]
    proc = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        raise RuntimeError(f"decode failed: {input_name}")
    return output


def read_pcm(path):
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        sample_rate = wav.getframerate()
        frames = wav.getnframes()
        data = wav.readframes(frames)

    samples = []
    if sample_width == 3:
        for i in range(0, len(data), 3):
            v = data[i] | (data[i + 1] << 8) | (data[i + 2] << 16)
            if v & 0x800000:
                v -= 1 << 24
            samples.append(v)
    elif sample_width == 2:
        for i in range(0, len(data), 2):
            v = data[i] | (data[i + 1] << 8)
            if v & 0x8000:
                v -= 1 << 16
            samples.append(v << 8)
    else:
        raise RuntimeError(f"unsupported sample width: {sample_width}")

    planes = [[samples[i] for i in range(ch, len(samples), channels)] for ch in range(channels)]
    return sample_rate, channels, sample_width * 8, frames, planes


def corr(a, b):
    mean_a = sum(a) / len(a)
    mean_b = sum(b) / len(b)
    cross = 0.0
    energy_a = 0.0
    energy_b = 0.0
    for x0, y0 in zip(a, b):
        x = x0 - mean_a
        y = y0 - mean_b
        cross += x * y
        energy_a += x * x
        energy_b += y * y
    if energy_a == 0.0 or energy_b == 0.0:
        return 0.0
    return cross / math.sqrt(energy_a * energy_b)


def assert_wav(path, expected_channels, expected_bits):
    sample_rate, channels, bits, frames, planes = read_pcm(path)
    if channels != expected_channels or bits != expected_bits:
        raise AssertionError(f"{path.name}: got {channels}ch/{bits}bit")
    nonzero = [sum(1 for x in plane if x) for plane in planes]
    if not any(nonzero):
        raise AssertionError(f"{path.name}: silent output")
    print(f"{path.name}: {sample_rate} Hz, {channels}ch, {bits}bit, frames={frames}")
    return planes


def assert_no_height_duplicates(path, threshold):
    planes = assert_wav(path, 10, 24)
    max_abs = 0.0
    for height in range(6, 10):
        if any(planes[height] == planes[base] for base in range(6)):
            raise AssertionError(f"{path.name}: height channel {height} is an exact base duplicate")
        best = max((corr(planes[height], planes[base]) for base in range(6)), key=abs)
        max_abs = max(max_abs, abs(best))
    print(f"  max abs height/base corr={max_abs:.6f}")
    if max_abs > threshold:
        raise AssertionError(f"{path.name}: height/base corr {max_abs:.6f} > {threshold}")


def main():
    if not EXE.exists():
        raise SystemExit(f"missing decoder: {EXE}")

    out_48_24 = run_decoder(
        "48hHz_6ch_auro_9.1(5.1+4H).wav",
        "regress_48hHz_6ch_auro_9.1_5.1_4H_pcm24.wav",
        24,
    )
    out_48_16 = run_decoder(
        "48hHz_6ch_auro_9.1(5.1+4H).wav",
        "regress_48hHz_6ch_auro_9.1_5.1_4H_pcm16.wav",
        16,
    )
    out_auro = run_decoder("auro.wav", "regress_auro_pcm24.wav", 24)
    out_2d = run_decoder("auro_2d.wav", "regress_auro_2d_pcm24.wav", 24)
    out_14 = run_decoder("7.1_5H1_1T.wav", "regress_7.1_5H1_1T_pcm24.wav", 24)

    assert_no_height_duplicates(out_48_24, 0.50)
    assert_wav(out_48_16, 10, 16)
    assert_no_height_duplicates(out_auro, 0.85)
    assert_wav(out_2d, 6, 24)
    assert_wav(out_14, 14, 24)
    print("regressions ok")


if __name__ == "__main__":
    main()
