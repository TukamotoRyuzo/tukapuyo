#pragma once
#include"platform.h"

// RKiss��hash key���v�Z���邽�߂Ɏg���[������������(PRNG)�ł���B
// George Marsagl�́A90�N��͂��߂�RNG-Kiss-family�𔭖������B
// �����Bob Jenkins��public domain�̃R�[�h����Heinz van Saanen���h��������
// ���ꉻ�o�[�W�����ł���BHeinz�ɂ��e�X�g���ꂽ�悤�ɁA�ȉ��̓���������B
// // / - ����߂ăv���b�g�t�H�[���Ɉˑ����Ȃ�
// / - ���ׂĂ�die harder�e�X�g���p�X�����I *nix(���@Unix�𕚂����ɂ��Ă���̂��H)��sys-rand()�Ȃ񂩂͔ߎS�Ȍ��ʂɂȂ�B
// / - *nix��sys-rand()���12�{���炢�����B
// / - SSE2-version of Mersenne twister��SSE2�ł��4�{���炢����
// / - ���ώ��� : 2^126���炢
// / - 64 bit�̗����V�[�h
// / - �t��53 bit�̉�������Ȃ�double�^��Ԃ���(���@�Öق̌^�ϊ��q�������Ă���̂ŉ��^�ł��Ԃ���)
// / - �X���b�h�Z�[�t 


class RKiss 
{   
	uint64_t a, b, c, d;

	uint64_t rotate(uint64_t x, uint64_t k) const 
	{    
		return (x << k) | (x >> (64 - k));  
	}   

	uint64_t rand64() 
	{     
		const uint64_t e = a - rotate(b, 7);
		a = b ^ rotate(c, 13);
		b = c + rotate(d, 37);
		c = d + e;
		return d = e + a;  
	} 
public:  
	// �R���X�g���N�^�B  
	// �����V�[�h��ύX���Ďg���Ȃ�ύX���Ďg���Ă��������c�B  
	RKiss(int seed = 73) 
	{     
		a = 0xF1EA5EED, b = c = d = 0xD4E12C77;

		for (int i = 0; i < seed; ++i)// Scramble a few rounds   
		{ 
			rand64();  
		}
	}   
	// �����������������o�����߂̈Öق̌^�ϊ��q�B  
	// int64_t�ȊO�ɂ�double�Ȃǂɂ�����ł���B  
	template<typename T> T rand() { return T(rand64()); }

}; 