#include"sound.h"

void Sound::load(const TCHAR *file_name)
{
	handle_ = DxLib::LoadSoundMem(file_name);
}

int Sound::handle()
{
	return handle_;
}

void Sound::play(int play_type)
{
	DxLib::PlaySoundMem(handle_, play_type);
}

void Sound::stop()
{
	DxLib::StopSoundMem(handle_);
}

// Ä¶ü”g”•ÏX
// -1‚ğ“n‚·‚ÆŒ³‚É–ß‚é
void Sound::changeFreq(int frequency)
{
	SetFrequencySoundMem(frequency, handle_); 
}

void Sound::changeVolume(int volume)
{
	DxLib::ChangeVolumeSoundMem(volume, handle_);
}