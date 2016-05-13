#pragma once


// MSVC�R���p�C������o�邤���Ƃ������x���𖳌��ɂ���B
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#pragma warning(disable: 4996) // Function _ftime() may be unsafe

// MSVC does not support <inttypes.h>
// MicroSoft Visual C++��inttypes.h���T�|�[�g���Ă��Ȃ��B�T�|�[�g���Ă���Ȃ炱���ɏ��������e��inttypes.h�ɔC����΂����B
typedef   signed __int8    int8_t;
typedef unsigned __int8   uint8_t;
typedef   signed __int16  int16_t;
typedef unsigned __int16 uint16_t;
typedef   signed __int32  int32_t;
typedef unsigned __int32 uint32_t;
typedef   signed __int64  int64_t;
typedef unsigned __int64 uint64_t;

#  include <sys/timeb.h>

// ���ݎ������~���b�P�ʂ܂ŋ��߂�B
inline int64_t system_time_to_msec() 
{
	_timeb t;
	_ftime(&t);
	return t.time * 1000LL + t.millitm;
}

#ifndef NOMINMAX
#  define NOMINMAX // disable macros min() and max()
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

// ���pifdef
#if _MSC_VER == 1600 
#define HAVE_SSE2
#define HAVE_SSE4
#endif

#if _MSC_VER >= 1900
#define HAVE_BMI2
#endif

// bsfq����(_Bitscanforword)���g�����ǂ���
#if defined(_WIN64) && !defined(IS_64BIT)
#  include <intrin.h> // MSVC popcnt and bsfq instrinsics
#  define IS_64BIT
#  define USE_BSF
#endif

//u64�ɑ΂���popcnt���߂��g����
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
// Windows 95�����98�̃p�����[�^lpThreadId�ł�null�ɂ��邱�Ƃ͂ł��܂���B
// my��may�̊ԈႢ�H
inline DWORD* dwWin9xKludge() { static DWORD dw; return &dw; }

// �N���e�B�J���Z�N�V�����I�u�W�F�N�g�̃��������蓖��
#  define lock_init(x) InitializeCriticalSection(&(x))

// �N���e�B�J���Z�N�V�����ɓ���
#  define lock_grab(x) EnterCriticalSection(&(x))

// �N���e�B�J���Z�N�V�������甲����
#  define lock_release(x) LeaveCriticalSection(&(x))

// �N���e�B�J���Z�N�V�����I�u�W�F�N�g�̍폜
#  define lock_destroy(x) DeleteCriticalSection(&(x))

// �C�x���g�I�u�W�F�N�g�̍쐬
#  define cond_init(x) { x = CreateEvent(0, FALSE, FALSE, 0); }

// �C�x���g�I�u�W�F�N�g���폜
#  define cond_destroy(x) CloseHandle(x)

// �C�x���g���V�O�i����Ԃɂ���@�ҋ@�֐�(WaitFor�Z�Z�֐�)�Ɉ���������̂���V�O�i�����
#  define cond_signal(x) SetEvent(x)

// �C�x���g���V�O�i����ԂɂȂ�܂ő҂i�i���Ɂj
#  define cond_wait(x,y) { lock_release(y); WaitForSingleObject(x, INFINITE); lock_grab(y); }

// �C�x���g���V�O�i����ԂɂȂ�܂ő҂iz�~���b�j
#  define cond_timedwait(x,y,z) { lock_release(y); WaitForSingleObject(x,z); lock_grab(y); }

// �X���b�h�̍쐬
#  define thread_create(x,f,t) (x = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)f,t,0,dwWin9xKludge()))

// �킩���
#  define thread_join(x) { WaitForSingleObject(x, INFINITE); CloseHandle(x); }

