#pragma once
#include <cassert>
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef min
#undef max
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "rkiss.h"
#include <chrono>

void MyOutputDebugString(LPCSTR pszFormat, ...);

// T�^��Ԃ�random�֐�
template <typename T>
T random(int max)
{
	// �c�����N�����ƂɕύX�������Ȃ��ꍇ�̓V�[�h���Œ�
	//static RKiss rk(24);
	static RKiss rk(static_cast<int>(time(NULL)) % 65535);
	return static_cast<T>(rk.rand<uint32_t>() % max);
}

// �ƂĂ����m�Ȏ��Ԃ��v���
class HighPrecisionTimer
{

public:

	// ���������Ɏ��s
	HighPrecisionTimer() { QueryPerformanceFrequency(&counter_freq_); }

	// ���Ԍv���i�}�C�N���P�ʁj
	void begin() { QueryPerformanceCounter(&before_counter_); }

	void end() { QueryPerformanceCounter(&counter_); }

	double elapsed()
	{
		return (double)(counter_.QuadPart - before_counter_.QuadPart) / (double)counter_freq_.QuadPart;
	}

private:

	// �b�Ԃ̃J�E���^��
	LARGE_INTEGER counter_freq_;

	// �O�̃J�E���^��
	LARGE_INTEGER before_counter_;

	// ���݂̃J�E���^��
	LARGE_INTEGER counter_;
};

// ms�P�ʂł̎��Ԍv�������K�v�Ȃ��̂ł����TimePoint�^�̂悤�Ɉ����B
typedef std::chrono::milliseconds::rep TimePoint;

// ms�P�ʂŌ��ݎ�����Ԃ�
inline TimePoint now()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::steady_clock::now().time_since_epoch()).count();
}

class TimeManagement
{
public:

	int elapsed() const { return int(now() - startTime); }
	void reset() { startTime = now(); }

private:
	TimePoint startTime;
};

extern TimeManagement Time;