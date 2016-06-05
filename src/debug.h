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

// �~���b�P�ʂ̎��Ԃ�\���N���X
class Time
{
public:
    Time() { restart(); }

	// �v���J�n���Ԃ�ۑ�
    void restart() { start_ = timeGetTime(); }

	// ���X�^�[�g����̕b����Ԃ�
    int elapsed()    
    {
        DWORD end = timeGetTime();
        return (int)(end - start_);
    }

private:
    DWORD start_;    //  �v���J�n����
};