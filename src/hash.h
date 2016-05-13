#pragma once
#include"const.h"
#include"platform.h"
#include"rkiss.h"

namespace {
	RKiss rk;
}
// �n�b�V���l�𔭐�������N���X
class Hashseed
{

public:
	// �R���X�g���N�^
	Hashseed()
	{
		for (int x = 0; x < FILE_MAX; x++)
			for (int y = 0; y < FH; y++)
				for (int color = 0; color < COLOR_MAX; color++)
					sq_color_rand[x * 16 + y][color] = rk.rand<uint64_t>();


		for (int t = 0; t < TUMO_MAX; t++)
			tumo_number_rand[t] = rk.rand<uint64_t>();
		
		allclear_rand = rk.rand<uint64_t>();
	}

	// �Տ�̋�̃n�b�V���l
	uint64_t seed(const Square sq, const Color color) const { return sq_color_rand[sq][color >> 4]; }
	uint64_t seed(const int tumo_number) const { return tumo_number_rand[tumo_number]; }
	uint64_t seed() const { return allclear_rand; }

private:
	
	// �t�B�[���h�̃n�b�V���V�[�h
	uint64_t sq_color_rand[SQUARE_MAX][COLOR_MAX];

	// ���݂̃c���ԍ��ɑ΂���n�b�V���V�[�h
	uint64_t tumo_number_rand[TUMO_MAX];

	// �S�����t���O�ɑ΂���n�b�V���V�[�h
	uint64_t allclear_rand;

};