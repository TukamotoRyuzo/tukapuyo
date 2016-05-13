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
	static RKiss rk(rand());
	return static_cast<T>(rk.rand<uint32_t>() % max);
}

// �ƂĂ����m�Ȏ��Ԃ��v���
class TimeCounter
{
private:

	// �b�Ԃ̃J�E���^��
	LARGE_INTEGER m_pCounterFreq;

	// �O�̃J�E���^��
	LARGE_INTEGER m_pBeforeCounter;

	// ���݂̃J�E���^��
	LARGE_INTEGER ulCounter;

public:

	TimeCounter()
	{
		// ���������Ɏ��s
		QueryPerformanceFrequency(&m_pCounterFreq);
	}

	void begin()
	{
		// ���Ԍv���i�}�C�N���P�ʁj
		QueryPerformanceCounter(&m_pBeforeCounter);
	}

	void end()
	{
		QueryPerformanceCounter(&ulCounter);
	}

	double elapsed()
	{
		return (double)(ulCounter.QuadPart - m_pBeforeCounter.QuadPart) / (double)m_pCounterFreq.QuadPart;
	}
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