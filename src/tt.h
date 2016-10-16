#pragma once
#include"platform.h"
#include"const.h"
#include"field.h"
#include<cstdlib>

struct TTEntry
{
	
	// ���̃N���X�̃C���X�^���X��calloc�Ŋm�ۂ����̂ŁA�R���X�g���N�^���Ăяo����邱�Ƃ͂Ȃ��B  
	// TTEntry*�𓾂āA�����ɑ΂���save()�ȂǂŒl���������񂾂肷��

	// �ǖʂ����ʂ��邽�߂ɕK�v�ȏ���
	// 1.�����̃t�B�[���h�̃n�b�V���L�[
	// 2.����̃t�B�[���h�̃n�b�V���L�[
	// 3.���݂̃c���ԍ�(�ő�127: 7bit)
	// 4.�����̎c�莞��(�ő�1337 : ���11bit)
	// 5.�\��������܂Ղ�̐�(�ő�2000: ���11bit)
	// 6.�����A���肪�S�����������Ă��邩�ǂ���

	// �ł���B�����������̃n�b�V���V�[�h��p�ӂ��n�b�V���L�[�ɉ�����̂͂��Ȃ��ԁB
	// �n�b�V���L�[�ɂ�1,2,3,6���l�������V�[�h�l��XOR���Ă���(�������Ȃ��ƈႤ�ǖʂ𓯂��ǖʂƂ��ēo�^����\���������Ȃ�)
	// �ǖʂ̃n�b�V���L�[�ȊO�̓G���g���[�ɓo�^���邱�Ƃɂ���D
	// �ǖʂ̃n�b�V���L�[�͎���^����ł���

	// �ȉ��Agetter

	uint32_t key()        const { return key32_;        }
	Move     move()       const { return Move(move16_); }
	Score    score()      const { return Score(score_); }
	uint8_t  depth()      const { return depth_;        }
	uint8_t  generation() const { return generation_;   }
	Bound    bound()      const { return Bound(bound_); }
	Flag     player()     const { return Flag(player_); }
	int16_t  time()       const { return time_;         }
	int16_t  ojama()      const { return ojama_;        }

	void save(Key k, uint16_t m, int16_t s, uint8_t d, uint8_t g, uint8_t b, uint8_t p, int16_t rt, int16_t o)
	{
		if (m || (k >> 32) != key32_)
			move16_ = m;

		if ((k >> 32) != key32_
			|| d > depth_ - 4
			|| b == BOUND_EXACT)
		{
			key32_ = (uint32_t)(k >> 32);
			move16_ = m;
			score_ = s;
			depth_ = d;
			generation_ = g;
			bound_ = b;
			player_ = p;
			time_ = rt;
			ojama_ = o;
		}
	}

	// ����̐ݒ�p
	void setGeneration(uint8_t g) { generation_ = g; }
private:

	friend class TranspositionTable;

	uint32_t key32_;
	int16_t move16_;
	int16_t score_;
	uint8_t depth_;
	uint8_t generation_;
	uint8_t bound_;
	uint8_t player_;
	int16_t time_;
	int16_t ojama_;

	// ���v16�o�C�g
};

class TranspositionTable 
{
	static const int CACHE_LINE_SIZE = 64;
	static const int CLUSTER_SIZE = 4;

	struct Cluster { TTEntry entry[CLUSTER_SIZE]; };

	static_assert(CACHE_LINE_SIZE % sizeof(Cluster) == 0, "Cluster size incorrect");

public:
	~TranspositionTable() { free(mem_); }
	void newSearch() { ++generation8_; }
	uint8_t generation() const { return generation8_; }

	template <bool OnePlayer>
	bool probe(const LightField* self, const LightField* enemy, TTEntry* &ptt) const;
	void resize(size_t mb_size);
	void clear() { memset(table_, 0, cluster_count_ * sizeof(Cluster)); generation8_ = 0; }
	TTEntry* firstEntry(const Key key) const { return &table_[(size_t)key & (cluster_count_ - 1)].entry[0]; }

private:
	// �u���\��index��mask�p�B  
	size_t cluster_count_;

	// �u���\�̐擪�������|�C���^�[(�m�ۂ�����������64�o�C�g�ŃA���C����������)
	Cluster* table_;

	// �u���\�̂��߂Ɋm�ۂ������̃������ւ̃|�C���^�[�B�J������Ƃ��ɕK�v�B
	void* mem_;

	// �u���\��Entry�́A���܎g���Ă��鐢��B  
	// �����root�ǖʂ��i�ނ��ƂɃC���N�������g���Ă����B
	uint8_t generation8_; 
};

extern TranspositionTable TT;

// Size �� 2�ׂ̂���ł��邱�ƁB
template <typename T, size_t Size>
struct HashTable 
{
	HashTable() : entries_(Size, T()) {}
	T* operator [] (const Key k) { return &entries_[static_cast<size_t>(k) & (Size - 1)]; }
	void clear() { std::fill(std::begin(entries_), std::end(entries_), T()); }
	// Size �� 2�ׂ̂���ł��邱�Ƃ̃`�F�b�N
	static_assert((Size & (Size - 1)) == 0, "");

private:
	std::vector<T> entries_;
};

const size_t EvaluateTableSize = 0x1000000; // 134MB
const size_t ChainsInfoTableSize = 0x1000000; // 134MB
// 64bit �ϐ�1�Ȃ̂͗��R�������āA
// �f�[�^���擾���Ă���Œ��ɑ��̃X���b�h���珑���������邱�Ƃ������Ȃ邩��B
// lockless hash �ƌĂ΂��B
// 128bit �Ƃ���������A64bit����2��f�[�^���擾���Ȃ��Ƃ����Ȃ��̂ŁA
// key �� score ���Ή����Ȃ��\��������B
// transposition table �͐��ɂ��̖�������Ă��邪�A
// �ÓI�]���l�̂悤�ɍ����]���������ł͂Ȃ��̂ŁA���ɂȂ邱�Ƃ͏��Ȃ��B
// 64bit�Ɏ��܂�Ȃ��ꍇ��Atransposition table �Ȃǂ����S�Ɉ��������Ȃ�A
// lock����ASSE��AVX���߂��g���A�`�F�b�N�T������������Akey �𕡐��̕ϐ��ɕ����ĕێ�����Ȃǂ̕��@������B
// 32bit OS �ꍇ�A64bit �ϐ��� 32bit����2��f�[�^�擾����̂ŁA����32bit�����32bit��xor����
// �f�[�^�擾���ɉ��Ă��Ȃ����m�F����B
// 31- 0 keyhigh32
// 60-32 score
// 61-63 remain help
struct EvaluateHashEntry
{
	uint32_t key() const { return static_cast<uint32_t>(word); }
	Score score() const { return static_cast<Score>(static_cast<int64_t>(word & 0x1fffffffffffffffULL) >> 32); }
	int remainHelp() const { return static_cast<int>(word >> 61);}

	void save(const Key k, const Score s, const int remain_help) 
	{
		// score��29�`31�r�b�g�ڂ�1�ɂȂ邱�Ƃ͑z�肵�Ă��Ȃ��D�܂��X�R�A���}�C�i�X�ɂȂ邱�Ƃ��z�肵�Ă��Ȃ��D
		assert(!(s & 0xe0000000) && s >= 0);
		word = static_cast<Key>(k >> 32) 
			 | static_cast<Key>(static_cast<int64_t>(s) << 32)
			 | static_cast<Key>(remain_help) << 61;
	}

	Key word;
};

struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};

// �t�B�[���h�̘A�����
struct ChainsInfo
{
	void save(const Key k, const Score s, const ChainsList* pcl)
	{
		key = k >> 32;
		max_score = s;
		cl = *pcl;
	}

	ChainsList cl;
	Score max_score;
	uint32_t key;
};

struct ChainsInfoTable : HashTable<ChainsInfo, ChainsInfoTableSize> {};
extern EvaluateHashTable ET;
extern ChainsInfoTable CIT;