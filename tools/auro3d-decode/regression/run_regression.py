#!/usr/bin/env python3
"""Regression tests for auro3d-decode: decode test files and compare SHA256 hashes."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import subprocess
import sys
import tempfile
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
CASES_PATH = Path(__file__).resolve().parent / "cases.json"
BASELINE_PATH = Path(__file__).resolve().parent / "baseline.json"


def load_cases() -> dict:
    with CASES_PATH.open(encoding="utf-8") as f:
        return json.load(f)


def resolve(path_text: str, base: Path) -> Path:
    path = Path(path_text)
    if not path.is_absolute():
        path = (base / path).resolve()
    return path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def wav_stats(path: Path) -> dict:
    with wave.open(str(path), "rb") as wav:
        frames = wav.getnframes()
        channels = wav.getnchannels()
        rate = wav.getframerate()
        width = wav.getsampwidth()
    return {
        "channels": channels,
        "sample_rate": rate,
        "sample_width": width,
        "frames": frames,
    }


def validate_cx_pcm(path: Path, case: dict) -> dict:
    with wave.open(str(path), "rb") as wav:
        stats = {
            "channels": wav.getnchannels(),
            "sample_rate": wav.getframerate(),
            "sample_width": wav.getsampwidth(),
            "frames": wav.getnframes(),
        }
        has_signal = False
        while True:
            pcm = wav.readframes(4096)
            if not pcm:
                break
            if any(pcm):
                has_signal = True

    expected = case["pcm"]
    for key in ("channels", "sample_rate", "sample_width"):
        if stats[key] != expected[key]:
            raise RuntimeError(
                f"{case['id']}: {key} mismatch "
                f"(expected {expected[key]}, got {stats[key]})"
            )
    if stats["frames"] <= 0:
        raise RuntimeError(f"{case['id']}: decoded PCM is empty")
    if not has_signal:
        raise RuntimeError(f"{case['id']}: decoded PCM is entirely silent")
    validate_cx_correlations(path, case)
    return stats


def pcm24(data: bytes, offset: int) -> int:
    value = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16)
    return value - 0x1000000 if value & 0x800000 else value


def validate_cx_correlations(path: Path, case: dict) -> None:
    limits = case.get("max_abs_correlations", [])
    if not limits:
        return
    accumulators = [[0, 0, 0, 0, 0, 0] for _ in limits]
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        if wav.getsampwidth() != 3:
            raise RuntimeError(f"{case['id']}: correlation check requires PCM24")
        for limit in limits:
            if not (0 <= limit["a"] < channels and 0 <= limit["b"] < channels):
                raise RuntimeError(f"{case['id']}: correlation channel out of range")
        frame_bytes = channels * 3
        while True:
            data = wav.readframes(4096)
            if not data:
                break
            for frame in range(len(data) // frame_bytes):
                base = frame * frame_bytes
                for index, limit in enumerate(limits):
                    x = pcm24(data, base + limit["a"] * 3)
                    y = pcm24(data, base + limit["b"] * 3)
                    values = accumulators[index]
                    values[0] += 1
                    values[1] += x
                    values[2] += y
                    values[3] += x * x
                    values[4] += y * y
                    values[5] += x * y
    for limit, values in zip(limits, accumulators):
        count, sum_x, sum_y, sum_x2, sum_y2, sum_xy = values
        covariance = count * sum_xy - sum_x * sum_y
        variance_x = count * sum_x2 - sum_x * sum_x
        variance_y = count * sum_y2 - sum_y * sum_y
        if variance_x <= 0 or variance_y <= 0:
            raise RuntimeError(f"{case['id']}: correlation channel is silent")
        correlation = covariance / math.sqrt(variance_x * variance_y)
        if abs(correlation) > limit["max"]:
            raise RuntimeError(
                f"{case['id']}: channels {limit['a']}/{limit['b']} correlation "
                f"{correlation:.6f} exceeds {limit['max']:.6f}"
            )


def run_case(decoder: Path, case: dict, output_dir: Path) -> dict:
    case_id = case["id"]
    input_path = resolve(case["input"], ROOT)
    if case.get("comparison") == "cx-consume":
        if not input_path.is_file():
            raise FileNotFoundError(f"input not found: {input_path}")
        if "pcm" in case:
            with tempfile.TemporaryDirectory(prefix="auro3d-cx-") as temp_dir:
                output_path = Path(temp_dir) / f"{case_id}.wav"
                cmd = [
                    str(decoder),
                    "-i",
                    str(input_path),
                    "-o",
                    str(output_path),
                ]
                proc = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    encoding="utf-8",
                    errors="replace",
                )
                if proc.returncode != 0:
                    raise RuntimeError(
                        f"decode failed for {case_id} (exit {proc.returncode})\n"
                        f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}"
                    )
                for expected_text in case.get("stderr_contains", []):
                    if expected_text not in proc.stderr:
                        raise RuntimeError(
                            f"{case_id}: stderr is missing expected text: "
                            f"{expected_text!r}\nstderr:\n{proc.stderr}"
                        )
                stats = validate_cx_pcm(output_path, case)
        else:
            cmd = [str(decoder), "-i", str(input_path), "-o", os.devnull]
            proc = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if proc.returncode != 0:
                raise RuntimeError(
                    f"decode failed for {case_id} (exit {proc.returncode})\n"
                    f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}"
                )
            stats = {}
        return {
            "id": case_id,
            "input": str(input_path.relative_to(ROOT)).replace("\\", "/"),
            "comparison": "cx-consume",
            **stats,
        }

    output_path = output_dir / f"{case_id}.wav"

    if not input_path.is_file():
        raise FileNotFoundError(f"input not found: {input_path}")

    cmd = [str(decoder), "-i", str(input_path), "-o", str(output_path), *case.get("args", [])]
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if proc.returncode != 0:
        raise RuntimeError(
            f"decode failed for {case_id} (exit {proc.returncode})\n"
            f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}"
        )
    if not output_path.is_file():
        raise RuntimeError(f"decoder did not produce output: {output_path}")

    stats = wav_stats(output_path)
    return {
        "id": case_id,
        "input": str(input_path.relative_to(ROOT)).replace("\\", "/"),
        "output": str(output_path.relative_to(ROOT)).replace("\\", "/"),
        "sha256": sha256_file(output_path),
        **stats,
    }


def run_all(cases_cfg: dict, output_dir: Path) -> dict:
    decoder = resolve(cases_cfg["decoder"], ROOT)
    if not decoder.is_file():
        raise FileNotFoundError(f"decoder not found: {decoder}")

    output_dir.mkdir(parents=True, exist_ok=True)
    results = []
    for case in cases_cfg["cases"]:
        print(f"  {case['id']}...", flush=True)
        results.append(run_case(decoder, case, output_dir))
    return {"decoder": str(decoder.relative_to(ROOT)).replace("\\", "/"), "cases": results}


def write_baseline(data: dict) -> None:
    with BASELINE_PATH.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"Baseline saved: {BASELINE_PATH}")


def compare_with_baseline(current: dict, baseline: dict) -> list[str]:
    errors: list[str] = []
    baseline_by_id = {c["id"]: c for c in baseline.get("cases", [])}
    for case in current["cases"]:
        case_id = case["id"]
        if case.get("comparison") == "cx-consume":
            continue
        ref = baseline_by_id.get(case_id)
        if ref is None:
            errors.append(f"{case_id}: missing in baseline")
            continue
        if case["sha256"] != ref["sha256"]:
            errors.append(f"{case_id}: sha256 mismatch\n  expected: {ref['sha256']}\n  got:      {case['sha256']}")
        for key in ("channels", "sample_rate", "sample_width", "frames"):
            if case.get(key) != ref.get(key):
                errors.append(
                    f"{case_id}: {key} mismatch (expected {ref.get(key)}, got {case.get(key)})"
                )
    missing = set(baseline_by_id) - {c["id"] for c in current["cases"]}
    for case_id in sorted(missing):
        errors.append(f"{case_id}: present in baseline but not run")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="auro3d-decode regression tests")
    parser.add_argument(
        "mode",
        choices=("baseline", "check"),
        help="baseline: capture golden hashes; check: compare against baseline",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="override output directory from cases.json",
    )
    parser.add_argument(
        "--cx-only",
        action="store_true",
        help="run only strict AuroCX bitstream-consumption cases",
    )
    args = parser.parse_args()

    cases_cfg = load_cases()
    if args.cx_only:
        cases_cfg = dict(cases_cfg)
        cases_cfg["cases"] = [
            case
            for case in cases_cfg["cases"]
            if case.get("comparison") == "cx-consume"
        ]
    output_dir = args.output_dir or resolve(cases_cfg["output_dir"], ROOT)

    print(f"Running {len(cases_cfg['cases'])} cases...")
    current = run_all(cases_cfg, output_dir)

    if args.mode == "baseline":
        if args.cx_only:
            print(f"OK: all {len(current['cases'])} AuroCX cases passed consumption and PCM invariants")
            return 0
        write_baseline(current)
        print("OK: baseline captured")
        return 0

    if args.cx_only:
        print(f"OK: all {len(current['cases'])} AuroCX cases passed consumption and PCM invariants")
        return 0

    if not BASELINE_PATH.is_file():
        print(f"Baseline not found: {BASELINE_PATH}", file=sys.stderr)
        print("Run: python tools/auro3d-decode/regression/run_regression.py baseline", file=sys.stderr)
        return 2

    with BASELINE_PATH.open(encoding="utf-8") as f:
        baseline = json.load(f)

    errors = compare_with_baseline(current, baseline)
    if errors:
        print("REGRESSION FAIL:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print(f"OK: all {len(current['cases'])} cases match baseline")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
