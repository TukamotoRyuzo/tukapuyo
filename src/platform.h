#pragma once


// MSVCコンパイラから出るうっとうしい警告を無効にする。
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#pragma warning(disable: 4996) // Function _ftime() may be unsafe

// MSVC does not support <inttypes.h>
// MicroSoft Visual C++はinttypes.hをサポートしていない。サポートしているならここに書かれる内容はinttypes.hに任せればいい。
typedef   signed __int8    int8_t;
typedef unsigned __int8   uint8_t;
typedef   signed __int16  int16_t;
typedef unsigned __int16 uint16_t;
typedef   signed __int32  int32_t;
typedef unsigned __int32 uint32_t;
typedef   signed __int64  int64_t;
typedef unsigned __int64 uint64_t;

#ifndef NOMINMAX
#  define NOMINMAX // disable macros min() and max()
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

// 俺用ifdef
#if _MSC_VER == 1600 
#define HAVE_SSE2
#define HAVE_SSE4
#endif

#if _MSC_VER >= 1900
#define HAVE_BMI2
#endif

// bsfq命令(_Bitscanforword)を使うかどうか
#if defined(_WIN64) && !defined(IS_64BIT)
#  include <intrin.h> // MSVC popcnt and bsfq instrinsics
#  define IS_64BIT
#  define USE_BSF
#endif

//u64に対するpopcnt命令を使うか
#if defined(_MSC_VER) && defined(IS_64BIT)
#define USE_POPCNT
#define HAVE_SSE42
#define HAVE_SSE4
#endif

#if defined (HAVE_BMI2)
#include <immintrin.h>
#endif

#if defined (HAVE_SSE42)
#include <nmmintrin.h>
#endif

#if defined (HAVE_SSE4)
#include <smmintrin.h>
#elif defined (HAVE_SSE2)
#include <emmintrin.h>
#endif

// #define POLYTEC_FESTA
