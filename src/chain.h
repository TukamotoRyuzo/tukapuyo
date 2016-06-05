#pragma once
#include "platform.h"
#include "const.h"
#include "bitboard.h"
#include <cassert>
#include <vector>

// �A���̃X�R�A�����߂�v�f�̃N���X�B�A���֐����Ő錾����Ƃ�₱�����̂łЂƂ܂Ƃ߂ɂ����B
struct ChainElement
{
	// �R���X�g���N�^�ł�chain���Z�b�g���Ă����B����0�N���A
	ChainElement(const int chain) : 
		chain_(static_cast<uint8_t>(chain)), 
		puyo_num_(0),
		color_bonus_(0),
		connect_bonus_(0)
	{};
	
	ChainElement() {}

	// �A���̃X�R�A���v�Z����
	Score chainScore() const { return static_cast<Score>(puyo_num_ * 10 * (CHAIN_BONUS[chain_] + connect_bonus_ + color_bonus_)); }
	int puyoNum() const { return puyo_num_; }
	// ���̎O��K���ĂԁD
	void setPuyoNum(const int p) { puyo_num_ = static_cast<uint8_t>(p); }
	void setColorBonus(const int c) { assert(c > 0); color_bonus_ = COLOR_BONUS[c - 1]; }
	void connectPlus(const int con) { connect_bonus_ += CONNECT_BONUS[(con - 4) & 7]; }

	// �A���e�[�u������l�𓾂��Ƃ��p
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
	// ���ז�������
	Score ojama;

	// ���t���[�������邩
	int time;

	// �A��������Ŕ��΂ł���̂��B1�Ȃ炱�̎�Ŕ��΂ł���B2�Ȃ獡�̎��u�������̃c���Ŕ��΂ł���B
	int hakka_frame;

public:
	Chains() {};
	Chains(const Chains& c) : ojama(c.ojama), time(c.time), hakka_frame(c.hakka_frame) {};
	Chains(const int o, const int t, const int c) : ojama(static_cast<Score>(o)), time(t), hakka_frame(c) {};
	bool operator == (Chains &c) const { return ojama == c.ojama && time == c.time && hakka_frame == c.hakka_frame; }
};

typedef std::vector<Chains> ChainsList;

// ���ۂɋǖʂ�i�߂����Ƃ���C�_�������m�肽���Ƃ��Ɏg���D
// 1�G���g���[32byte�ɂȂ�͂�
// ���ۂɋǖʂ�i�߂�Ƃ���bbDelete()���Ăяo���Ώ�����ꏊ��T������K�v�͂Ȃ��D
// �_�������m�肽���Ƃ���afterKey()���Ăяo���Cprobe���邱�Ƃ�nullptr�ɐ���܂ŌJ��Ԃ��Όv�Z�ł���D
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

	// ���̋ǖʂŏ�����ꏊ��1�ɂȂ��Ă���bitboard
	Bitboard bb_delete_;

	// ��������̋ǖʂ̃n�b�V���L�[
	Key after_key_;

	// ���̋ǖʂ̃n�b�V���L�[�̏��32�r�b�g
	uint32_t key_;

	// ���̋ǖʂŋN�������A���v�f
	ChainElement ce_;
};

class ChainTable
{
	static const unsigned CLUSTER_SIZE = 2;
	const int CACHE_LINE_SIZE = 64;
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