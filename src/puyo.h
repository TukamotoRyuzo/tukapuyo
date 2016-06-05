#pragma once

#include "platform.h"
#include "enumoperator.h"
#include <cassert>
#include <string>

const int TUMO_MAX = 128;

enum Square
{
	SQ_ZERO = 0,
	A0 = 16, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15,
	B0, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14, B15,
	C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15,
	D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, 
	E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15,
	F0, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
	SQUARE_MAX = 128,

	DEADPOINT = C12,
	
	// �����Z�����Z�̂Ƃ��֗��Ȃ̂Œ�`���Ă����B
	SQ_UP = 1,
	SQ_UP2 = 2,
	SQ_DOWN = -1, 
	SQ_DOWN2 = -2,
	SQ_RIGHT = 16,
	SQ_LEFT = -16,
};

ENABLE_OPERATORS_ON(Square);

enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_MAX };
enum Rank { RANK_0, RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9, RANK_10, RANK_11, RANK_12, RANK_13, RANK_14, RANK_15, RANK_MAX };

ENABLE_OPERATORS_ON(File);
ENABLE_OPERATORS_ON(Rank);

inline bool isInSquare(const int x, const int y)
{
	// y��0�������B�Ղ�Ղ悪�S��������Ă��Ȃ��������Ƃ��Aupper(x) - 1�Ƃ����Q�Ƃ����邱�Ƃ����邩��B
	return x >= 1 && x <= 6 && y >= 0 && y <= RANK_MAX;
}

inline Square toSquare(const int x, const int y) { assert(isInSquare(x, y)); return static_cast<Square>(x * 16 + y); }
inline File toX(const Square sq) { return static_cast<File>(sq >> 4); }
inline Rank toY(const Square sq) { return static_cast<Rank>(sq & 0x0f); }
inline bool isInSquare(const Square sq) { return isInSquare(toX(sq), toY(sq)); }

inline std::string squareToStr(const Square sq)
{
	const char x = 'A' + toX(sq) - 1;
	const char y = toY(sq) <= 9 ? '0' + toY(sq) : 'A' + toY(sq) - 10;
	std::string s;
	s = x;
	s += y;
	return s;
}

enum ColorType { TRED, TGREEN, TBLUE, TYELLOW, TPURPLE, TOJAMA, NCOLOR_MAX = TOJAMA, };

// Connect������4�r�b�g�ƂȂ�ق����������悢�̂ŁAColor�̓V�t�g���Ă����B
enum Color
{
	EMPTY 		= 0,
	WALL 		= 1 << 4,
	RED			= 2 << 4,
	BLUE		= 3 << 4,
	YELLOW		= 4 << 4,
	GREEN 		= 5 << 4,
	PURPLE 		= 6 << 4,
	OJAMA		= 7 << 4,
	COLOR_MAX	= 8,
};

inline Color colorTypeToColor(const ColorType ct) { return static_cast<Color>((ct + 2) << 4); }
inline ColorType colorToColorType(const Color c) { return static_cast<ColorType>((c >> 4) - 2); }

// ��]���ɑ΂��Ďq�Ղ悪�ǂ��ɂ��邩�������B
enum Rotate { ROTATE_UP, ROTATE_RIGHT, ROTATE_DOWN, ROTATE_LEFT, ROTATE_MAX };

ENABLE_OPERATORS_ON(Rotate);

const int ROTATE_SQUARE[4] = { 1, 16, -1, -16 };
const int LSB_SQUARE[16] = { 0, 1, 16, 1, -1, 1, 16, 1, -16, 1, 16, 1, -1, 1, 16, 1 };

inline Square rotatePosition(const Rotate r) { assert(r >= ROTATE_UP && r <= ROTATE_LEFT); return static_cast<Square>(ROTATE_SQUARE[r]); }

enum Connect
{
	CON_NONE  = 0,
	CON_UP	  = 1 << ROTATE_UP,
	CON_RIGHT = 1 << ROTATE_RIGHT,
	CON_DOWN  = 1 << ROTATE_DOWN,
	CON_LEFT  = 1 << ROTATE_LEFT,
};

inline Square lsbToPosition(const Connect c) { return static_cast<Square>(LSB_SQUARE[c]); }

ENABLE_OPERATORS_ON(Connect);

// Color��Connect�����킹�����̂�"Puyo"�ƌĂԂ��Ƃɂ���B
typedef uint8_t Puyo;

// ���ݑ��쒆�́APuyo������킳�������̂�"Tumo"�ƌĂԂ��Ƃɂ���B
class Tumo
{

public:
	Tumo(){};

	Tumo(const Square sq, const Color col0, const Color col1, const Rotate rot) { init(sq, col0, col1, rot); }

	// �R���X�g���N�^�ȊO�ł��������ł���悤�ɁB
	void init(const Square sq, const Color col0, const Color col1, const Rotate rot)
	{
		sq_ = sq;
		color_[0] = col0;
		color_[1] = col1;
		rotate_ = rot;
	}

	void init(const Square sq, const ColorType ct1, const ColorType ct2, const Rotate rot)
	{
		sq_ = sq;
		color_[0] = colorTypeToColor(ct1);
		color_[1] = colorTypeToColor(ct2);
		rotate_ = rot;
	}

	void setColor(const Color c1, const Color c2)
	{
		color_[0] = c1;
		color_[1] = c2;
	}

	// getter
	// p : ���Ղ�ipivot�j, c : �q�Ղ�ichild�j��\���B
	Square psq() const { return sq_; }
	Square csq() const { return sq_ + rotatePosition(rotate_); }
	Color pColor() const { return color_[0]; }
	Color cColor() const { return color_[1]; }
	Color color(const int index) const { assert(index == 0 || index == 1);  return color_[index]; }
	File px() const { return static_cast<File>(psq() >> 4); }
	File cx() const { return static_cast<File>(csq() >> 4); }
	Rank py() const { return static_cast<Rank>(psq() & 0x0f); }
	Rank cy() const { return static_cast<Rank>(csq() & 0x0f); }

	void setSquare(const Square sq) { sq_ = sq; }
	void setRotate(const Rotate r) { rotate_ = r; }

	// �q�Ղ�̂��������Ԃ��D
	Rotate rotate() const { return rotate_; }

	// �q�Ղ�̂�������Ƃ͋t�̕�����Ԃ�.
	Rotate rotateRev() const { return static_cast<Rotate>((rotate_ + 2) & 3); }

	// �q�Ղ悪���ɂ���Ȃ�true.
	bool isSide() const { return rotate_ & 1; }

	// �E��]������D
	void rightRotate() { rotate_ = static_cast<Rotate>((rotate_ + 1) & 3); }

	// ����]������D
	void leftRotate() { rotate_ = static_cast<Rotate>((rotate_ + 3) & 3); }

	// ���ړ�
	void down() { --sq_; }	

	// ��ړ�
	void up() { ++sq_; }

	// �E�ړ�
	void right() { sq_ += SQ_RIGHT; }

	// ���ړ�
	void left() { sq_ += SQ_LEFT; }

	// �������������ړ��D
	void addx(const File x) { sq_ += static_cast<Square>(16 * x); }

	bool operator==(const Tumo &p) const 
	{
		return sq_ == p.sq_ 
		|| color_[0] == p.color_[0] 
		|| color_[1] == p.color_[1]
		|| rotate_ == p.rotate_;
	}

	bool operator!=(const Tumo &p) const { return !((*this) == p); }

private:

	// ���Ղ�̈ʒu
	Square sq_;

	// color_[0]�����Ղ�̐F�Acolor_[1]���q�Ղ�̐F
	Color color_[2];

	// ���݂̎��Ղ悩�猩���q�Ղ�̂������
	Rotate rotate_; 

};// class Tumo

