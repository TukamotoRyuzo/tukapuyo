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

// ms単位での時間計測しか必要ないのでこれをTimePoint型のように扱う。
typedef std::chrono::milliseconds::rep TimePoint;

// ms単位で現在時刻を返す
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