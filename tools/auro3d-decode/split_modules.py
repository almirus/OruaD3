#!/usr/bin/env python3
"""Split monolithic auro3deng_from_ida.cpp into module files."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "auro3deng_from_ida.cpp"
OUT_DIR = ROOT / "src" / "auro3deng"

# Split at boundaries where the merged anonymous namespace does not cross files.
# codec_dsp: full DSP path through codec-v3 process / output generator.
# v4_android: JNI/Android A3DENG API layer.
# asc4he: ASC4HE / CenterGen processors.
MODULES: list[tuple[str, int, int, str]] = [
    ("codec_dsp.cpp", 207, 11762, "Codec v3, A3DENG v3/v4 pipeline, AuroMatic, downmix"),
    ("v4_android.cpp", 11763, 13852, "Android A3DENG API"),
    ("asc4he.cpp", 13854, 16874, "ASC4HE / CenterGen"),
]

CONSTANTS_START = 70
CONSTANTS_END = 205

PREAMBLE_HPP = """#pragma once

#include "auro3deng_from_ida.hpp"
#include "auro3deng_processor_io.hpp"
#include "auro3deng_processor_offsets.hpp"
#include "auro_codec_v3_ida.hpp"
#include "auro3deng_internal_constants.hpp"

#ifdef round
#undef round
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)) && !defined(_M_ARM64)
#if defined(__SSE4_1__) || defined(__AVX2__) || defined(_MSC_VER)
#define AURO3DENG_CRC_SSE41 1
#include <smmintrin.h>
#endif
#endif
#ifndef AURO3DENG_CRC_SSE41
#define AURO3DENG_CRC_SSE41 0
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)) && !defined(_M_ARM64)
#include <emmintrin.h>
#define AURO3DENG_SSE2_PLATFORM 1
#else
#define AURO3DENG_SSE2_PLATFORM 0
#endif
"""

CONSTANTS_HPP = """#pragma once

namespace auro3deng {

{body}

} // namespace auro3deng
"""


def read_lines() -> list[str]:
    return SRC.read_text(encoding="utf-8").splitlines(keepends=True)


def slice_lines(all_lines: list[str], start: int, end: int) -> str:
    return "".join(all_lines[start - 1 : end])


def write_module(name: str, body: str, extra_before_ns: str = "") -> None:
    content = '#include "auro3deng_internal_preamble.hpp"\n\n'
    if extra_before_ns:
        content += extra_before_ns + "\n"
    content += "namespace auro3deng {\n\n"
    content += body
    if not body.endswith("\n"):
        content += "\n"
    content += "\n} // namespace auro3deng\n"
    (OUT_DIR / name).write_text(content, encoding="utf-8")


def main() -> None:
    if not SRC.is_file():
        raise SystemExit(f"source not found: {SRC}")

    lines = read_lines()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (ROOT / "auro3deng_internal_preamble.hpp").write_text(PREAMBLE_HPP, encoding="utf-8")

    constants_body = slice_lines(lines, CONSTANTS_START, CONSTANTS_END).rstrip()
    (ROOT / "auro3deng_internal_constants.hpp").write_text(
        "#pragma once\n\nnamespace auro3deng {\n\n" + constants_body + "\n\n} // namespace auro3deng\n",
        encoding="utf-8",
    )
    print(f"  auro3deng_internal_constants.hpp: lines {CONSTANTS_START}-{CONSTANTS_END}")

    # Remove stale modules from the earlier 10-way split attempt.
    for old in OUT_DIR.glob("*.cpp"):
        old.unlink()

    centergen_fwd = slice_lines(lines, 62, 66)

    for name, start, end, desc in MODULES:
        body = slice_lines(lines, start, end)
        extra = centergen_fwd if name == "asc4he.cpp" else ""
        write_module(name, body, extra)
        print(f"  {name}: lines {start}-{end} ({end - start + 1} lines) — {desc}")

    print(f"Created {len(MODULES)} modules in {OUT_DIR}")


if __name__ == "__main__":
    main()
