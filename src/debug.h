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
	// ツモを起動ごとに変更したくない場合はシードを固定
	//static RKiss rk(24);
	static RKiss rk(static_cast<int>(time(NULL)) % 65535);
	return static_cast<T>(rk.rand<uint32_t>() % max);
}

// とても正確な時間が計れる
class HighPrecisionTimer
{

public:

	// 初期化時に実行
	HighPrecisionTimer() { QueryPerformanceFrequency(&counter_freq_); }

	// 時間計測（マイクロ単位）
	void begin() { QueryPerformanceCounter(&before_counter_); }

	void end() { QueryPerformanceCounter(&counter_); }

	double elapsed()
	{
		return (double)(counter_.QuadPart - before_counter_.QuadPart) / (double)counter_freq_.QuadPart;
	}

private:

	// 秒間のカウンタ数
	LARGE_INTEGER counter_freq_;

	// 前のカウンタ数
	LARGE_INTEGER before_counter_;

	// 現在のカウンタ数
	LARGE_INTEGER counter_;
};

// ミリ秒単位の時間を表すクラス
class Timer
{
public:
    Timer() { restart(); }

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