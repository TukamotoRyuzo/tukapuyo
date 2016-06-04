#pragma once

#include"DxLib.h"

class Fps
{

public:

	Fps() : start_time_(0), count_(0), fps_(0) {};

	bool update()
	{
		// 1�t���[���ڂȂ玞�����L��
		if (count_ == 0)
			start_time_ = GetNowCount();
		
		// 60�t���[���ڂȂ畽�ς��v�Z����
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
		// ������������
		int took_time = GetNowCount() - start_time_;	

		// �҂ׂ�����
		int wait_time = count_*1000/FPS - took_time;

		// �ҋ@
		if (wait_time > 0)
			Sleep(wait_time);	
	}

private:
	
	// ����J�n����
	int start_time_;

	// �J�E���^
	int count_;   

	// fps
	float fps_;

	// ���ς����T���v����
	static const int N = 60;

	// �ݒ肵��FPS
	static const int FPS = 60;

};
