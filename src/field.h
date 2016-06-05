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

// AI用に無駄な変数を省いたサイズの小さなフィールド
class LightField
{
public:
	// アクセサ
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
	Key key() const { return hash_key_; }
	Tumo getDepthTumo(const int depth) const { return tumo_pool_[(tumoIndex() + depth) & 127]; }
	Tumo getTumo(const int id) const { return tumo_pool_[id]; }
	Tumo getNowTumo() const { return getTumo(tumoIndex()); }
	Tumo getPreviousTumo() const { return getTumo(previousTumoIndex()); }
	Flag player() const { return flag(PLAYER_1OR2); }
	int getPuyoNum() const { return field_puyo_num_; }
	// ツモ番号を進め、ハッシュを計算する。
	void nextPlus() 
	{ 
		hash_key_ ^= hash_->seed(tumoIndex()); 
		hash_key_ ^= hash_->seed(nextTumoIndex()); 
		next_ = (next_ + 1) & 127; 
	}

	// ツモ番号を戻し、ハッシュを計算する。
	void nextMinus()
	{ 
		next_ = next_ - 1 & 127;
		hash_key_ ^= hash_->seed(tumoIndex());
		hash_key_ ^= hash_->seed(nextTumoIndex()); 
	}

protected:

	// コネクト情報のセット、リセット　clear,Reculはコネクト情報の再計算が必要な部分だけの再計算
	void setConnect(const Square sq, int rotate = -1);
	void setConnectMin(const Square sq);
	void resetConnect(const Square sq);
	bool clearConnect();
	void setConnectRecul();

	// おじゃまぷよを降らす。表示は考慮しない
	void ojamaFallMin(int *score);

	// 相殺
	void offseting(LightField &enemy);

	// ぷよを消す
	template <bool Hash> void deletePuyo(Bitboard& bb);

	// sqにいるぷよがいくつ連結しているのか
	int findSameColor(Bitboard &bb, const Square sq) const;
	void setSameColor(Bitboard &bb, const Square sq) const;
	Bitboard bbSameColor(const Square sq) const;
	
	void aiInit();

	// 自分に与えられた時間からボーナス点を求める。マイナスもありえる。
	Score timeAndChainsToScore(const Score self_score, const Score enemy_score, const int remain_time, const LightField& enemy)const;

	// ツモの範囲を決める
	void setRange(int* left, int* right) const;

	// フィールドの見る範囲を決める。
	void setRange2(int* left, int* right) const;

public:
	LightField() {};
	explicit LightField(Flag f) :flag_(f) {};
	explicit LightField(const Field &f);

	// Fieldのvalidator
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
	Key keyInit();
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

	// 自分にとって振ったら死ぬお邪魔の量を返す。
	Score deadLine() const { return static_cast<Score>(6 * (13 - upper(3))); }
	Score connectBonus(int con1, int con2, int con3) const { return static_cast<Score>(con3 * connect_[2] + con2 * connect_[1] + con1 * connect_[0]); }
	void saveConnect(int *con_prev) const { con_prev[0] = connect_[0]; con_prev[1] = connect_[1]; con_prev[2] = connect_[2]; }
	bool operator==(const LightField &f) const;

	Puyo puyo(const Square sq) const { return field_[sq]; }

	// このゲームで使う色を記憶しておく。
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

	// フィールド．壁を含む．
	Puyo field_[SQUARE_MAX];

	// 現在おかれているぷよの一番上の座標（x軸ごとに用意)
	Rank upper_y_[FILE_MAX];

	// connect情報の再計算の必要範囲
	Rank recul_y_[FILE_MAX];

	// 現局面の連結度をあらわす.たとえばconnect_[2]は3つつながっている箇所がいくつあるか、という情報が入っている
	int connect_[3];

	// さまざまなフラグをここで使いまわす
	Flag flag_;

	// 現在の点数（おじゃまぷよが振るたびにリセットする）
	int score_;

	// 自分に降るおじゃまぷよ
	int ojama_;
	
	// 現局面の位置の点数
	int position_bonus_;

	// 次のぷよ配列番号
	int next_;
	
	// 現在の連鎖数
	int chain_;

	// 局面のハッシュ値
	Key hash_key_;
	
	// field上にあるおじゃまぷよ以外のぷよの数と，おじゃまぷよの数
	int field_puyo_num_, field_ojama_num_;

	// 1p,2pでそれぞれ別のハッシュシードを用意するため
	const Hashseed *hash_;

	// 128ツモ
	const Tumo* tumo_pool_;
	const uint8_t* ojama_pool_;
	
	// AI用::敵に読み取らせるための値
	int chain_max_, score_max_;

	// フィールドが持っている連鎖のリスト
	ChainsList* cl_;

	uint8_t ojama_rand_id_;

	static Color use_color_[4];
};

// 表示情報をプラスしたフィールド。
// このクラスの動作は高速である必要は全くないので極力わかりやすく書くべき。
class Field : public LightField
{
private:
	bool putTumo(const Tumo &dest);// ぷよを移動させるための関数群
	void dropTumo();
	bool fall();// ちぎり
	bool slide();// 連鎖後のぷよを落とす
	bool canDelete();
	bool vanish();// connect情報を利用した高速版
	void deleteMark();
	void ojamaFall(Field &enemy);// おじゃまぷよがふる

	int wait(Field &enemy);
	int processInput(Operate *ope);// キー入力管理。もしくはAIの手を管理する。
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
	void init();// 初期化
	void show();// 表示
	bool procedure(Field &enemy, Operate *ope = nullptr);// ゲーム管理関数
	int generateStaticState(Field& enemy);
	Tumo current() const { return current_; }

private:

	// 描画を細かく見せるための値
	float x_offset_;
	float freedrop_offset_;
	float drop_offset_;
	float rotate_offset_x_, rotate_offset_y_;
	float move_offset_;

	int fall_w_;
	int chain_stage_, puyon_stage_;

	// 連鎖が起こったときのもっとも右下の位置
	Square chain_right_bottom_;

	// 合計点数
	int score_sum_;

	// タイマ
	int wait_;		

	// 次にSETしたとき降るおじゃまぷよ
	int ojama_buf_;

	// ちぎりが発生した位置と、残ったほうの位置
	Square tigiri_, remain_;

	// 操作に関する制御用変数
	int r_count_, l_count_, r_rotate_count_, l_rotate_count_, down_count_;

	int up_limit_;
	int prev_y_;

	// 消えるぷよにマークをつけておくためのダミー配列
	int delete_mark_[SQUARE_MAX];

	// 画像類へのポインタ
	GameAssets* assets;

	// 現在の操作中のツモの情報
	Tumo current_;
};// class Field

inline int min3(int x, int y, int z) { return x < y ? x < z ? x : z : y < z ? y : z; }
inline int toDir(const Rotate rotate) { return 1 << rotate; }
inline int toDirRev(const Rotate rotate) { return 1 << ((rotate + 2) & 3); }

// たかだか4ビットのbsfには組み込み命令を使う必要はないと判断。
inline Rotate lsbToRotate(const Connect b) 
{ 
	static const int tbl[16] = { -1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };
	assert(b > 0 && b < 16 && tbl[b] != -1);
	return static_cast<Rotate>(tbl[b]);
}

namespace Bonus
{
	// 位置ボーナス点
	// field_[x][y]の位置におじゃまぷよ以外のぷよがあるときにボーナス点を与える
	// これによりツモ数による有利不利、おじゃまぷよの割合によるマイナスを評価できるはず
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

// template関数をcppで作りたかったがやり方がわからないのでここに定義するハメになった。
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

// 範囲を決める
inline void LightField::setRange(int* left, int* right) const
{
	*left = 1;
	*right = 6;

	// 調べる範囲を決める
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

// 範囲を決めるが、レンジは↑の関数より一つ広い。
inline void LightField::setRange2(int *left, int* right)const
{
	*left = 1;
	*right = 6;

	// 調べる範囲を決める
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