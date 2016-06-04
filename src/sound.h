#pragma once

#include"DxLib.h"

// 音に関するクラス
class Sound
{
private:
	int handle_;// 音楽ハンドル

public:
	void load(const TCHAR *FileName);
	void play(int playType = DX_PLAYTYPE_BACK);
	void stop();
	void changeFreq(int Frecency);
	void changeVolume(int volume);
	int handle();
};