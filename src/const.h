#pragma once

#include"enumoperator.h"
#include"platform.h"

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

	// もっとも長い連鎖時間
	TIME_MAX = CHAINTIME * 19 + NEXTTIME,
};

enum Score
{
	RATE = 70, // おじゃまレート
	OJAMALINE = RATE * 6, // ぷよが一列降る点数

	// ゼロ
	SCORE_ZERO = 0,

	// ひきわけ　
	SCORE_DRAW = 0,
	SCORE_KNOWN_WIN = 10000,
	SCORE_KNOWN_LOSE = -10000,
	// 詰み
	SCORE_MATE = 32000,
	SCORE_MATED = -32000,

	// 　∞
	SCORE_INFINITE = 32601,

	// それ以外の特殊な定数
	SCORE_NONE = 32602,

	// これらは探索用ではなく，連鎖の点数をお邪魔ぷよの個数に変える前の値の仮の限界値
	SCORE_MAX = 999999,
	SCORE_MIN = -999999,
};
ENABLE_OPERATORS_ON(Score);
enum 
{
	// PLAYER1のフィールドの左上のx,y座標
	P1_F_BEGINX = 48,
	P1_F_BEGINY = 48,

	// PLAYER1のNEXTの左上のx,y座標
	P1_N_BEGINX = 263,
	P1_N_BEGINY = 113,

	// PLAYER1のNEXTNEXTの左上のx,y座標
	P1_NN_BEGINX = 292,
	P1_NN_BEGINY = 143,

	// PLAYER2のフィールドの左上のx,y座標
	P2_F_BEGINX = 398,
	P2_F_BEGINY = 48,

	// PLAYER2のNEXTの左上のx,y座標
	P2_N_BEGINX = 348,
	P2_N_BEGINY = 113,

	// PLAYER2のNEXTNEXTの左上のx,y座標
	P2_NN_BEGINX = 334,
	P2_NN_BEGINY = 143,

	P_SIZE		= 32,			// ぷよの一辺の長さ
	WIN_WIDTH	= 640,			// ウィンドウサイズ:横幅
	WIN_HEIGHT	= 480,			// 高さはぷよ12個分
	F_WID		= 6,			// フィールドの幅
	F_HEI		= 13,			// フィールドの高さ
	F_HEI_VIS	= 12,			// フィールドの見える範囲
	FH			= 16,			// 高さ（番兵含める）
};

enum Flag
{
	// PLAYERをあらわすbit、WAITをあらわすbitがバラバラなのは逐次追加していったから
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


// スコア(評価値)がどういう探索の結果得た値なのかを表現する。
enum Bound 
{
	// 探索しておらず、局面の静的評価値か最善手のみを置換表に突っ込みたいときに用いる。 
	BOUND_NONE,

	// fail-lowしたときの状態。この局面のスコアの最大値であり、このnodeの真の評価値はttValue以下である。  
	// non PV nodeとか、bestMoveがない(適当な見積もりで枝刈りした場合など)は、この状態  
	BOUND_UPPER,

	// fail-highしたときの状態。この局面のスコアの最低保証値であり、このnodeの真の評価値はttValue以上である。  
	// beta cutしたときに、そのときの値を置換表に格納するときに使う。  
	// beta cutなので他の残りの指し手に関して、これよりいい評価値のものがあるはずだという意味で使う。  
	BOUND_LOWER,

	// fail-lowもfail-highもしておらず正確なスコアと言える状態  
	// 	PvNodeでかつbestMoveが具体的な指し手として存在する場合はこの状態  
	BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

/*
つながり方によるぷよの表示の切り替え配列
0000 0
0001 上 1
0010 右 2
0011 上右 5 
0100 下 3
0101 上下 6 
0110 右下 8 
0111 上右下 13 
1000 左 4 
1001 左上 7
1010 左右 9 
1011 左右上 11 
1100 左下 10 
1101 左下上 14 
1110 左下右 12 
1111 左下右上 15
*/
//const int con_color[16] = { 0, 1, 2, 5, 3, 6, 8, 13, 4, 7, 9, 11,10, 14, 12, 15};

/*
つながり方によるぷよの表示の切り替え配列
0000 0
0001 上 2
0010 右 4
0011 上右 6 
0100 下 1
0101 上下 3 
0110 右下 5 
0111 上右下 7 
1000 左 8 
1001 左上 10
1010 左右 12 
1011 左右上 14 
1100 左下 9 
1101 左下上 11 
1110 左下右 13 
1111 左下右上 15
*/
const int con_color[16] = { 0, 2, 4, 6, 1, 3, 5, 7, 8, 10, 12, 14, 9, 11, 13, 15};
ENABLE_OPERATORS_ON(Flag);

extern unsigned int eval_called;

extern unsigned int chain_hit;
extern unsigned int chain_called;
extern unsigned int node_searched;
extern unsigned int color_searched;
extern unsigned int allready_searched;