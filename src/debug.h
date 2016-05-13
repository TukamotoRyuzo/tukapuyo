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

// T型を返すrandom関数
template <typename T>
T random(int max)
{
	static RKiss rk(rand());
	return static_cast<T>(rk.rand<uint32_t>() % max);
}

// とても正確な時間が計れる
class TimeCounter
{
private:

	// 秒間のカウンタ数
	LARGE_INTEGER m_pCounterFreq;

	// 前のカウンタ数
	LARGE_INTEGER m_pBeforeCounter;

	// 現在のカウンタ数
	LARGE_INTEGER ulCounter;

public:

	TimeCounter()
	{
		// 初期化時に実行
		QueryPerformanceFrequency(&m_pCounterFreq);
	}

	void begin()
	{
		// 時間計測（マイクロ単位）
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

// ミリ秒単位の時間を表すクラス
class Time
{
public:
    Time() { restart(); }

	// 計測開始時間を保存
    void restart() { start_ = timeGetTime(); }

	// リスタートからの秒数を返す
    int elapsed()    
    {
        DWORD end = timeGetTime();
        return (int)(end - start_);
    }

private:
    DWORD start_;    //  計測開始時間
};