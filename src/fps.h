#pragma once

#include"DxLib.h"

class Fps
{

public:

	Fps() : start_time_(0), count_(0), fps_(0) {};

	bool update()
	{
		// 1フレーム目なら時刻を記憶
		if (count_ == 0)
			start_time_ = GetNowCount();
		
		// 60フレーム目なら平均を計算する
		if (count_ == N)
		{ 
			int t = GetNowCount();
			fps_ = 1000.f/((t-start_time_)/(float)N);
			count_ = 0;
			start_time_ = t;
		}
		count_++;
		return true;
	}

	void draw() const { DrawFormatString(0, 0, GetColor(255,255,255), "%.1f", fps_); }

	void wait() const 
	{
		// かかった時間
		int took_time = GetNowCount() - start_time_;	

		// 待つべき時間
		int wait_time = count_*1000/FPS - took_time;

		// 待機
		if (wait_time > 0)
			Sleep(wait_time);	
	}

private:
	
	// 測定開始時刻
	int start_time_;

	// カウンタ
	int count_;   

	// fps
	float fps_;

	// 平均を取るサンプル数
	static const int N = 60;

	// 設定したFPS
	static const int FPS = 60;

};
