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

//#define POLYTEC_FESTA

typedef CRITICAL_SECTION Lock;
typedef HANDLE WaitCondition;
typedef HANDLE NativeHandle;

// On Windows 95 and 98 parameter lpThreadId my not be null
// Windows 95および98のパラメータlpThreadIdではnullにすることはできません。
// myはmayの間違い？
inline DWORD* dwWin9xKludge() { static DWORD dw; return &dw; }

// クリティカルセクションオブジェクトのメモリ割り当て
#  define lock_init(x) InitializeCriticalSection(&(x))

// クリティカルセクションに入る
#  define lock_grab(x) EnterCriticalSection(&(x))

// クリティカルセクションから抜ける
#  define lock_release(x) LeaveCriticalSection(&(x))

// クリティカルセクションオブジェクトの削除
#  define lock_destroy(x) DeleteCriticalSection(&(x))

// イベントオブジェクトの作成
#  define cond_init(x) { x = CreateEvent(0, FALSE, FALSE, 0); }

// イベントオブジェクトを削除
#  define cond_destroy(x) CloseHandle(x)

// イベントをシグナル状態にする　待機関数(WaitFor〇〇関数)に引っかかるのが非シグナル状態
#  define cond_signal(x) SetEvent(x)

// イベントがシグナル状態になるまで待つ（永遠に）
#  define cond_wait(x,y) { lock_release(y); WaitForSingleObject(x, INFINITE); lock_grab(y); }

// イベントがシグナル状態になるまで待つ（zミリ秒）
#  define cond_timedwait(x,y,z) { lock_release(y); WaitForSingleObject(x,z); lock_grab(y); }

// スレッドの作成
#  define thread_create(x,f,t) (x = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)f,t,0,dwWin9xKludge()))

// わかんね
#  define thread_join(x) { WaitForSingleObject(x, INFINITE); CloseHandle(x); }

