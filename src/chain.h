#pragma once
#include "platform.h"
#include "const.h"
#include "bitboard.h"
#include <cassert>
#include <vector>

// 連鎖のスコアを決める要素のクラス。連鎖関数内で宣言するとややこしいのでひとまとめにした。
struct ChainElement
{
	// コンストラクタではchainをセットしておく。他は0クリア
	ChainElement(const int chain) : 
		chain_(static_cast<uint8_t>(chain)), 
		puyo_num_(0),
		color_bonus_(0),
		connect_bonus_(0)
	{};
	
	ChainElement() {}

	// 連鎖のスコアを計算する
	Score chainScore() const { return static_cast<Score>(puyo_num_ * 10 * (CHAIN_BONUS[chain_] + connect_bonus_ + color_bonus_)); }
	int puyoNum() const { return puyo_num_; }
	// この三つを必ず呼ぶ．
	void setPuyoNum(const int p) { puyo_num_ = static_cast<uint8_t>(p); }
	void setColorBonus(const int c) { assert(c > 0); color_bonus_ = COLOR_BONUS[c - 1]; }
	void connectPlus(const int con) { connect_bonus_ += CONNECT_BONUS[(con - 4) & 7]; }

	// 連鎖テーブルから値を得たとき用
	void set(const ChainElement* ce)
	{
		puyo_num_ = ce->puyo_num_;
		connect_bonus_ = ce->connect_bonus_;
		color_bonus_ = ce->color_bonus_;
	}

	int chain() const { return chain_; }
	void setChain(const int chain) { chain_ = chain; }
	void plusChain() { ++chain_; }
	const ChainElement* ref() const { return this; }
private:
	uint8_t puyo_num_;
	uint8_t chain_;

	uint8_t connect_bonus_;
	uint8_t color_bonus_;

	static const int CHAIN_BONUS[19];
	static const uint8_t CONNECT_BONUS[8];
	static const uint8_t COLOR_BONUS[5];
};

struct Chains
{
	// お邪魔何個分か
	Score ojama;

	// 何フレームかかるか
	int time;

	// 連鎖が何手で発火できるのか。1ならこの手で発火できる。2なら今の手を置いた次のツモで発火できる。
	int hakka_frame;

public:
	Chains() {};
	Chains(const Chains& c) : ojama(c.ojama), time(c.time), hakka_frame(c.hakka_frame) {};
	Chains(const int o, const int t, const int c) : ojama(static_cast<Score>(o)), time(t), hakka_frame(c) {};
	bool operator == (Chains &c) const { return ojama == c.ojama && time == c.time && hakka_frame == c.hakka_frame; }
};

typedef std::vector<Chains> ChainsList;

// 実際に局面を進めたいときや，点数だけ知りたいときに使う．
// 1エントリー32byteになるはず
// 実際に局面を進めるときはbbDelete()を呼び出せば消える場所を探索する必要はない．
// 点数だけ知りたいときはafterKey()を呼び出し，probeすることをnullptrに成るまで繰り返せば計算できる．
struct ChainEntry
{
	// getter
	uint32_t key() const { return key_; }
	const ChainElement* chainElement() const { return ce_.ref(); }
	Bitboard bbDelete() const { return bb_delete_; }
	Key afterKey() const { return after_key_; }

	// setter
	void save(uint32_t key, Key after_key, const ChainElement& ce, const Bitboard& bb)
	{
		key_ = key;
		after_key_ = after_key;
		ce_ = ce;
		bb_delete_ = bb;
	}
private:

	// この局面で消える場所が1になっているbitboard
	Bitboard bb_delete_;

	// 消えた後の局面のハッシュキー
	Key after_key_;

	// この局面のハッシュキーの上位32ビット
	uint32_t key_;

	// この局面で起こった連鎖要素
	ChainElement ce_;
};

class ChainTable
{
	static const unsigned CLUSTER_SIZE = 2;
public:
	~ChainTable() { free(mem_); }
	const ChainEntry* probe(const Key key) const ;
	void store(const Key key, Key after_key, const ChainElement& ce, const Bitboard& bb);
	ChainEntry* firstEntry(const Key key) const;
	void setSize(size_t mb_size);
	void clear();
private:

	uint32_t hash_mask_;
	ChainEntry* table_;
	void* mem_;
};

inline ChainEntry* ChainTable::firstEntry(const Key key) const {

	return table_ + ((uint32_t)key & hash_mask_);
}

extern ChainTable CT;