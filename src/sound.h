#pragma once

#include"DxLib.h"

// ���Ɋւ���N���X
class Sound
{
private:
	int handle_;// ���y�n���h��

public:
	void load(const TCHAR *FileName);
	void play(int playType = DX_PLAYTYPE_BACK);
	void stop();
	void changeFreq(int Frecency);
	void changeVolume(int volume);
	int handle();
};