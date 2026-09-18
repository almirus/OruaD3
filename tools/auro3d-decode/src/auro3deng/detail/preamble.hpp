#pragma once

#include "runtime_api.hpp"
#include "processor_io.hpp"
#include "processor_offsets.hpp"
#include "codec_v3.hpp"
#include "constants.hpp"

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
