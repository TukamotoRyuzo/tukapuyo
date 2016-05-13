#pragma once

#include"enumoperator.h"
#include"platform.h"
// �������̃L���b�V�����C���̃T�C�Y
// (1�o�C�g�ڂ�ǂݍ��ނƁA�㑱��64�o�C�g���ǂݍ��܂��B)
#define CACHE_LINE_SIZE 64
enum
{
	CHAINTIME	= 70,
	ONEPUT_TIME = 45,
	NEXTTIME	= 7,
	SETTIME		= 15,
	TIGIRITIME	= 8,
	FALLTIME	= 3,
	FREEDROPTIME = FALLTIME * 25 + 1,
	TAKETIME	= SETTIME + NEXTTIME,
	TAKETIME_INCLUDETIGIRI = TAKETIME + TIGIRITIME,

	// �����Ƃ������A������
	TIME_MAX = CHAINTIME * 19 + NEXTTIME,
};

enum Score
{
	RATE = 70, // ������܃��[�g
	OJAMALINE = RATE * 6, // �Ղ悪���~��_��

	// �[��
	SCORE_ZERO = 0,

	// �Ђ��킯�@
	SCORE_DRAW = 0,
	SCORE_KNOWN_WIN = 15000,
	SCORE_KNOWN_LOSE = -15000,
	// �l��
	SCORE_MATE = 30000,

	// �@��
	SCORE_INFINITE = 30001,

	// ����ȊO�̓���Ȓ萔
	SCORE_NONE = 30002,
	SCORE_WIN = SCORE_INFINITE,
	SCORE_LOSE = -SCORE_INFINITE,

	// �����͒T���p�ł͂Ȃ��C�A���̓_�������ז��Ղ�̌��ɕς���O�̒l�̉��̌��E�l
	SCORE_MAX = 999999,
	SCORE_MIN = -999999,
};
ENABLE_OPERATORS_ON(Score);
enum 
{
	// PLAYER1�̃t�B�[���h�̍����x,y���W
	P1_F_BEGINX = 48,
	P1_F_BEGINY = 48,

	// PLAYER1��NEXT�̍����x,y���W
	P1_N_BEGINX = 263,
	P1_N_BEGINY = 113,

	// PLAYER1��NEXTNEXT�̍����x,y���W
	P1_NN_BEGINX = 292,
	P1_NN_BEGINY = 143,

	// PLAYER2�̃t�B�[���h�̍����x,y���W
	P2_F_BEGINX = 398,
	P2_F_BEGINY = 48,

	// PLAYER2��NEXT�̍����x,y���W
	P2_N_BEGINX = 348,
	P2_N_BEGINY = 113,

	// PLAYER2��NEXTNEXT�̍����x,y���W
	P2_NN_BEGINX = 334,
	P2_NN_BEGINY = 143,

	P_SIZE		= 32,			// �Ղ�̈�ӂ̒���
	WIN_WIDTH	= 640,			// �E�B���h�E�T�C�Y:����
	WIN_HEIGHT	= 480,			// �����͂Ղ�12��
	F_WID		= 6,			// �t�B�[���h�̕�
	F_HEI		= 13,			// �t�B�[���h�̍���
	F_HEI_VIS	= 12,			// �t�B�[���h�̌�����͈�
	FH			= 16,			// �����i�ԕ��܂߂�j
};

enum Flag
{
	// PLAYER������킷bit�AWAIT������킷bit���o���o���Ȃ̂͒����ǉ����Ă���������
	SET				= 1 << 0,
	RENSA			= 1 << 1,
	TIGIRI			= 1 << 2,
	SLIDE			= 1 << 3,
	PLAYER2			= 1 << 4,
	PLAYER1			= 1 << 5,
	WAIT_SET		= 1 << 6,
	WAIT_SLIDE		= 1 << 7,
	WAIT_RENSA		= 1 << 8,
	WAIT_TIGIRI		= 1 << 9,
	WAIT_NEXT		= 1 << 10,
	WAIT_OJAMA		= 1 << 11,
	RENSA_END		= 1 << 12,
	OJAMA_FALLING	= 1 << 13,
	OJAMA_WILLFALL	= 1 << 14,
	ALLCLEAR		= 1 << 15,
	PLAYER_AI		= 1 << 16,
	OJAMAOK			= 1 << 17,
	OJAMA_RESERVE	= 1 << 18,
	VANISH			= 1 << 19,
	VANISH2			= 1 << 20,
	WAIT_TIGIRISET  = 1 << 21,
	WAIT_DROP		= 1 << 22,
	DROP			= 1 << 23,
	ZURU_OK			= 1 << 24,
	STATIC_MAKING   = 1 << 25,
	REPLAY_MODE     = 1 << 26,
	ZURU_MODE       = 1 << 27, 
	PLAYER_INFO		= PLAYER1 | PLAYER2 | PLAYER_AI,
	PLAYER_1OR2		= PLAYER1 | PLAYER2,
	WAIT			= WAIT_SET | WAIT_SLIDE | WAIT_RENSA | WAIT_TIGIRI | WAIT_NEXT | WAIT_OJAMA | WAIT_TIGIRISET | WAIT_DROP | ZURU_OK,	
};

enum 
{
	LEFT		= 1 << 0,
	RIGHT		= 1 << 1,
	DOWN		= 1 << 2,
	R_ROTATE	= 1 << 3,
	L_ROTATE	= 1 << 4,
};


// �X�R�A(�]���l)���ǂ������T���̌��ʓ����l�Ȃ̂���\������B
enum Bound 
{
	// �T�����Ă��炸�A�ǖʂ̐ÓI�]���l���őP��݂̂�u���\�ɓ˂����݂����Ƃ��ɗp����B 
	BOUND_NONE,

	// fail-low�����Ƃ��̏�ԁB���̋ǖʂ̃X�R�A�̍ő�l�ł���A����node�̐^�̕]���l��ttValue�ȉ��ł���B  
	// non PV node�Ƃ��AbestMove���Ȃ�(�K���Ȍ��ς���Ŏ}���肵���ꍇ�Ȃ�)�́A���̏��  
	BOUND_UPPER,

	// fail-high�����Ƃ��̏�ԁB���̋ǖʂ̃X�R�A�̍Œ�ۏؒl�ł���A����node�̐^�̕]���l��ttValue�ȏ�ł���B  
	// beta cut�����Ƃ��ɁA���̂Ƃ��̒l��u���\�Ɋi�[����Ƃ��Ɏg���B  
	// beta cut�Ȃ̂ő��̎c��̎w����Ɋւ��āA�����肢���]���l�̂��̂�����͂����Ƃ����Ӗ��Ŏg���B  
	BOUND_LOWER,

	// fail-low��fail-high�����Ă��炸���m�ȃX�R�A�ƌ�������  
	// 	PvNode�ł���bestMove����̓I�Ȏw����Ƃ��đ��݂���ꍇ�͂��̏��  
	BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

/*
�Ȃ�����ɂ��Ղ�̕\���̐؂�ւ��z��
0000 0
0001 �� 1
0010 �E 2
0011 ��E 5 
0100 �� 3
0101 �㉺ 6 
0110 �E�� 8 
0111 ��E�� 13 
1000 �� 4 
1001 ���� 7
1010 ���E 9 
1011 ���E�� 11 
1100 ���� 10 
1101 ������ 14 
1110 �����E 12 
1111 �����E�� 15
*/
//const int con_color[16] = { 0, 1, 2, 5, 3, 6, 8, 13, 4, 7, 9, 11,10, 14, 12, 15};

/*
�Ȃ�����ɂ��Ղ�̕\���̐؂�ւ��z��
0000 0
0001 �� 2
0010 �E 4
0011 ��E 6 
0100 �� 1
0101 �㉺ 3 
0110 �E�� 5 
0111 ��E�� 7 
1000 �� 8 
1001 ���� 10
1010 ���E 12 
1011 ���E�� 14 
1100 ���� 9 
1101 ������ 11 
1110 �����E 13 
1111 �����E�� 15
*/
const int con_color[16] = { 0, 2, 4, 6, 1, 3, 5, 7, 8, 10, 12, 14, 9, 11, 13, 15};
ENABLE_OPERATORS_ON(Flag);

extern unsigned int eval_called;

extern unsigned int chain_hit;
extern unsigned int chain_called;
extern unsigned int node_searched;
extern unsigned int color_searched;
extern unsigned int allready_searched;