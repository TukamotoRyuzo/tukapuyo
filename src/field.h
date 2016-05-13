#pragma once

#include "platform.h"
#include "puyo.h"
#include "hash.h"
#include "move.h"
#include <cassert>
#include <algorithm>
#include "GameAssets.h"
#include "chain.h"
#include "debug.h"
class Field;
class Bitboard;

// AI�p�ɖ��ʂȕϐ����Ȃ����T�C�Y�̏����ȃt�B�[���h
class LightField
{
public:
	// �A�N�Z�T
	void resetFlag(const Flag f) { flag_ = f; }
	void setFlag(const Flag f) { flag_ |= f; }
	void clearFlag(const Flag f) { flag_ &= ~f; }
	void setOjama(int o) { ojama_ = o; }
	void setChains(ChainsList *cl) { cl_ = cl; }
	ChainsList* chainsList() const { return cl_; }
	Flag flag(const Flag f) const { return flag_ & f; }
	Flag flag() const { return flag_; }
	Score score() const { return static_cast<Score>(score_); }
	int ojama() const { return ojama_; }
	Score positionBonus() const; 
	int upper(const int x) const { assert(x >= 0 && x <= 7); return upper_y_[x]; }
	int recul(const int x) const { assert(x >= 0 && x <= 7); return recul_y_[x]; }
	int nextTumoIndex() const { return next_; }
	int tumoIndex() const { return (next_ - 1) & 127; }
	int previousTumoIndex() const { return (next_ - 2) & 127; }
	int scoreMax() const { return score_max_; }
	int chainMax() const { return chain_max_; }
	int chain() const { return chain_; }
	uint64_t key() const { return hash_key_; }
	Tumo getDepthTumo(const int depth) const { return tumo_pool_[(tumoIndex() + depth) & 127]; }
	Tumo getTumo(const int id) const { return tumo_pool_[id]; }
	Tumo getNowTumo() const { return getTumo(tumoIndex()); }
	Tumo getPreviousTumo() const { return getTumo(previousTumoIndex()); }
	Flag player() const { return flag(PLAYER_1OR2); }
	int getPuyoNum() const { return field_puyo_num_; }
	// �c���ԍ���i�߁A�n�b�V�����v�Z����B
	void nextPlus() 
	{ 
		hash_key_ ^= hash_->seed(tumoIndex()); 
		hash_key_ ^= hash_->seed(nextTumoIndex()); 
		next_ = (next_ + 1) & 127; 
	}

	// �c���ԍ���߂��A�n�b�V�����v�Z����B
	void nextMinus()
	{ 
		next_ = next_ - 1 & 127;
		hash_key_ ^= hash_->seed(tumoIndex());
		hash_key_ ^= hash_->seed(nextTumoIndex()); 
	}

protected:

	// �R�l�N�g���̃Z�b�g�A���Z�b�g�@clear,Recul�̓R�l�N�g���̍Čv�Z���K�v�ȕ��������̍Čv�Z
	void setConnect(const Square sq, int rotate = -1);
	void setConnectMin(const Square sq);
	void resetConnect(const Square sq);
	bool clearConnect();
	void setConnectRecul();

	// ������܂Ղ���~�炷�B�\���͍l�����Ȃ�
	void ojamaFallMin(int *score);

	// ���E
	void offseting(LightField &enemy);

	// �Ղ������
	template <bool Hash> void deletePuyo(Bitboard& bb);

	// sq�ɂ���Ղ悪�����A�����Ă���̂�
	int findSameColor(Bitboard &bb, const Square sq) const;
	void setSameColor(Bitboard &bb, const Square sq) const;
	Bitboard bbSameColor(const Square sq) const;
	
	void aiInit();

	// �����ɗ^����ꂽ���Ԃ���{�[�i�X�_�����߂�B�}�C�i�X�����肦��B
	Score timeAndChainsToScore(const Score self_score, const Score enemy_score, const int remain_time, const LightField& enemy)const;

	// �c���͈̔͂����߂�
	void setRange(int* left, int* right) const;

	// �t�B�[���h�̌���͈͂����߂�B
	void setRange2(int* left, int* right) const;

public:
	LightField() {};
	explicit LightField(Flag f) :flag_(f) {};
	explicit LightField(const Field &f);

	// Field��validator
	int examine() const;
	bool isEmpty();
	bool isAllClear() const;
	int bonusInit();
	bool deleteMin();
	Score deleteScore();
	Score deleteScoreDestroy();
	int generateMoves(Move* buf) const;
	template <bool Evaluating> void doMove(const Move &move);
	int doMoveEX(const Move& move, int my_remain_time, LightField& enemy);
	void undoMove(const Move& move, int *con_prev);
	void undoMove(const Move& move);
	uint64_t keyInit();
	Score evaluate(LightField& enemy, int remain_time);

	Score maxChainsScore(const int ply_max, const int remain_time);
	Score searchChains(const int ply_max);
	Score searchChains(const int ply_max, int ply);
	Score chainBonus(const int connect_num) const;
	Score colorHelper(const int help_max);
	Score colorHelper(const int help_max, int help_num);
	template <bool Hash> void slideMin();
	bool isEmpty(const Square sq) const { return !puyo(sq); }
	bool isOpen(const Square sq) const { return (isEmpty(sq + SQ_RIGHT) || isEmpty(sq + SQ_LEFT) || isEmpty(sq + SQ_UP)); }
	Color color(const Square sq) const { return Color(puyo(sq) & 0xf0); }
	Connect connect(const Square sq) const { return Connect(puyo(sq) & 0x0f); }
	bool isDeath() const { return puyo(DEADPOINT) != EMPTY; }

	// �����ɂƂ��ĐU�����玀�ʂ��ז��̗ʂ�Ԃ��B
	Score deadLine() const { return static_cast<Score>(6 * (13 - upper(3))); }
	Score connectBonus(int con1, int con2, int con3) const { return static_cast<Score>(con3 * connect_[2] + con2 * connect_[1] + con1 * connect_[0]); }
	void saveConnect(int *con_prev) const { con_prev[0] = connect_[0]; con_prev[1] = connect_[1]; con_prev[2] = connect_[2]; }
	bool operator==(const LightField &f) const;

	Puyo puyo(const Square sq) const { return field_[sq]; }

	// ���̃Q�[���Ŏg���F���L�����Ă����B
	static void setUseColor(const Color except_color)
	{
		static const Color c[5] = { RED, GREEN, BLUE, YELLOW, PURPLE };
		int n = 0;

		for (int i = 0; i < 5; i++)
		{
			if (c[i] != except_color)
			{
				use_color_[n++] = c[i];
			}
		}
	}

	static Color useColor(const int index) { return use_color_[index]; }

protected:

	// �t�B�[���h�D�ǂ��܂ށD
	Puyo field_[SQUARE_MAX];

	// ���݂�����Ă���Ղ�̈�ԏ�̍��W�ix�����Ƃɗp��)
	Rank upper_y_[FILE_MAX];

	// connect���̍Čv�Z�̕K�v�͈�
	Rank recul_y_[FILE_MAX];

	// ���ǖʂ̘A���x������킷.���Ƃ���connect_[2]��3�Ȃ����Ă���ӏ����������邩�A�Ƃ�����񂪓����Ă���
	int connect_[3];

	// ���܂��܂ȃt���O�������Ŏg���܂킷
	Flag flag_;

	// ���݂̓_���i������܂Ղ悪�U�邽�тɃ��Z�b�g����j
	int score_;

	// �����ɍ~�邨����܂Ղ�
	int ojama_;
	
	// ���ǖʂ̈ʒu�̓_��
	int position_bonus_;

	// ���̂Ղ�z��ԍ�
	int next_;
	
	// ���݂̘A����
	int chain_;

	// �ǖʂ̃n�b�V���l
	uint64_t hash_key_;
	
	// field��ɂ��邨����܂Ղ�ȊO�̂Ղ�̐��ƁC������܂Ղ�̐�
	int field_puyo_num_, field_ojama_num_;

	// 1p,2p�ł��ꂼ��ʂ̃n�b�V���V�[�h��p�ӂ��邽��
	const Hashseed *hash_;

	// 128�c��
	const Tumo* tumo_pool_;
	const uint8_t* ojama_pool_;
	
	// AI�p::�G�ɓǂݎ�点�邽�߂̒l
	int chain_max_, score_max_;

	// �t�B�[���h�������Ă���A���̃��X�g
	ChainsList* cl_;

	uint8_t ojama_rand_id_;

	static Color use_color_[4];
};

// �\�������v���X�����t�B�[���h�B
// ���̃N���X�̓���͍����ł���K�v�͑S���Ȃ��̂ŋɗ͂킩��₷�������ׂ��B
class Field : public LightField
{
private:
	bool putTumo(const Tumo &dest);// �Ղ���ړ������邽�߂̊֐��Q
	void dropTumo();
	bool fall();// ������
	bool slide();// �A����̂Ղ�𗎂Ƃ�
	bool canDelete();
	bool vanish();// connect���𗘗p����������
	void deleteMark();
	void ojamaFall(Field &enemy);// ������܂Ղ悪�ӂ�

	int wait(Field &enemy);
	int processInput(Operate *ope);// �L�[���͊Ǘ��B��������AI�̎���Ǘ�����B
	int simuraterConvert(int score);
	void set(const Tumo &p);
	void setEmpty(const Tumo &p);

public:
	Field(){};
	explicit Field(Flag f, GameAssets* s) : LightField(f), assets(s)
	{ 
		tumo_pool_ = assets->tumo_pool; 
		ojama_pool_ = assets->ojama_rand_; 
	}

	void tumoReload();
	void init();// ������
	void show();// �\��
	bool procedure(Field &enemy, Operate *ope = nullptr);// �Q�[���Ǘ��֐�
	int generateStaticState(Field& enemy);
	Tumo current() const { return current_; }

private:

	// �`����ׂ��������邽�߂̒l
	float x_offset_;
	float freedrop_offset_;
	float drop_offset_;
	float rotate_offset_x_, rotate_offset_y_;
	float move_offset_;

	int fall_w_;
	int chain_stage_, puyon_stage_;

	// �A�����N�������Ƃ��̂����Ƃ��E���̈ʒu
	Square chain_right_bottom_;

	// ���v�_��
	int score_sum_;

	// �^�C�}
	int wait_;		

	// ����SET�����Ƃ��~�邨����܂Ղ�
	int ojama_buf_;

	// �����肪���������ʒu�ƁA�c�����ق��̈ʒu
	Square tigiri_, remain_;

	// ����Ɋւ��鐧��p�ϐ�
	int r_count_, l_count_, r_rotate_count_, l_rotate_count_, down_count_;

	int up_limit_;
	int prev_y_;

	// ������Ղ�Ƀ}�[�N�����Ă������߂̃_�~�[�z��
	int delete_mark_[SQUARE_MAX];

	// �摜�ނւ̃|�C���^
	GameAssets* assets;

	// ���݂̑��쒆�̃c���̏��
	Tumo current_;
};// class Field

inline int min3(int x, int y, int z) { return x < y ? x < z ? x : z : y < z ? y : z; }
inline int toDir(const Rotate rotate) { return 1 << rotate; }
inline int toDirRev(const Rotate rotate) { return 1 << ((rotate + 2) & 3); }

// ��������4�r�b�g��bsf�ɂ͑g�ݍ��ݖ��߂��g���K�v�͂Ȃ��Ɣ��f�B
inline Rotate lsbToRotate(const Connect b) 
{ 
	static const int tbl[16] = { -1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };
	assert(b > 0 && b < 16 && tbl[b] != -1);
	return static_cast<Rotate>(tbl[b]);
}

namespace Bonus
{
	// �ʒu�{�[�i�X�_
	// field_[x][y]�̈ʒu�ɂ�����܂Ղ�ȊO�̂Ղ悪����Ƃ��Ƀ{�[�i�X�_��^����
	// ����ɂ��c�����ɂ��L���s���A������܂Ղ�̊����ɂ��}�C�i�X��]���ł���͂�
	const int position_bonus_seed[SQUARE_MAX] =
	{
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  3,  3,  3,  2,  2,  2,  2,  2, -1, -3, -9,  -15, -20,  0,  0,
		0,  3,  3,  3,  2,  2,  2,  1,  1, -2, -4, -12, -17, -22,  0,  0,
		0,  2,  2,  2,  1,  1,  1, -1, -2, -3, -6, -15, -45, SCORE_KNOWN_LOSE,  0,  0,
		0,  2,  2,  2,  1,  1,  1,  1,  0, -2, -4, -12, -17, -22,  0,  0,
		0,  3,  3,  3,  2,  2,  2,  1,  1, -1, -3, -9,  -15, -20,  0,  0,
		0,  3,  3,  3,  2,  2,  2,  2,  2,  0, -2, -6,  -9,  -15,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,    0,  0,  0
	};
/*	const int position_bonus_seed[SQUARE_MAX] =
	{
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  1,  1,  1,  0,  0,  0,  0,  0, -1, -3, -9,  -15, -20,  0,  0,
		0,  1,  1,  1,  0,  0,  0,  -1,  -1, -2, -4, -12, -17, -22,  0,  0,
		0,  0,  0,  0,  -1, -1,  -1, -1, -2, -3, -6, -15, -45, SCORE_KNOWN_LOSE,  0,  0,
		0,  0,  0,  0,  -1  -1,  -1,  -1,  -2, -2, -4, -12, -17, -22,  0,  0,
		0,  1,  1,  1,  0,  0,  0,  -1,  -1, -1, -3, -9,  -15, -20,  0,  0,
		0,  1,  1,  1,  0,  0,  0,  0,  0,  0, -2, -6,  -9,  -15,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,    0,  0,  0
	};*/
	const int position_ojama_penalty[SQUARE_MAX] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 0, 0,
		0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 0, 0,
		0, -1, -2, -3, -4, -5, -6, -7, -13, -19, -25, -100, SCORE_KNOWN_LOSE, 0, 0,
		0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 0, 0,
		0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 0, 0,
		0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

}// namespace Bonus

// template�֐���cpp�ō�肽���������������킩��Ȃ��̂ł����ɒ�`����n���ɂȂ����B
template <bool Evaluating> inline void LightField::doMove(const Move &move)
{
	const Square psq = move.psq();
	const Square csq = move.csq();
	const Tumo now = getPreviousTumo();
	const Color pc = now.pColor();
	const Color cc = now.cColor();

	field_[psq] = pc;

	if (Evaluating)
		setConnect(psq);
	else
		setConnectMin(psq);

	field_[csq] = cc;

	if (Evaluating)
		setConnect(csq);
	else
		setConnectMin(csq);

	++upper_y_[toX(psq)];
	++upper_y_[toX(csq)];

	hash_key_ ^= hash_->seed(psq, pc);
	hash_key_ ^= hash_->seed(csq, cc);

	if (Evaluating)
	{
		position_bonus_ += Bonus::position_bonus_seed[psq];
		position_bonus_ += Bonus::position_bonus_seed[csq];
	}
}

// �͈͂����߂�
inline void LightField::setRange(int* left, int* right) const
{
	*left = 1;
	*right = 6;

	// ���ׂ�͈͂����߂�
	for (int x = 2; x >= 1; x--)
	{
		if (upper(x) >= 13)
		{
			*left = x + 1;
			break;
		}
	}
	for (int x = 4; x <= 6; x++)
	{
		if (upper(x) >= 13)
		{
			*right = x - 1;
			break;
		}
	}
}

// �͈͂����߂邪�A�����W�́��̊֐�����L���B
inline void LightField::setRange2(int *left, int* right)const
{
	*left = 1;
	*right = 6;

	// ���ׂ�͈͂����߂�
	for (int x = 2; x >= 1; x--)
	{
		if (upper(x) >= 13)
		{
			*left = x ;
			break;
		}
	}
	for (int x = 4; x <= 6; x++)
	{
		if (upper(x) >= 13)
		{
			*right = x;
			break;
		}
	}
}