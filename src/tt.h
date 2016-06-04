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
	// �Ӗ��́���private���Q�Ƃ���

	uint32_t        key() const { return key32_; }
	Move           move() const { return Move(move16_); }
	Score         score() const { return static_cast<Score>(score_); }
	uint8_t remainDepth() const { return remain_depth_; }
	uint8_t  generation() const { return generation_; }
	Bound         bound() const { return static_cast<Bound>(bound_); }
	Flag         player() const { return static_cast<Flag>(player_);}
	int16_t        time() const { return time_; }
	int16_t       ojama() const { return ojama_; }

	void save(uint32_t key, uint16_t move, int16_t score, uint8_t remain_depth, uint8_t gen, uint8_t bound, uint8_t player,
		int16_t remain_time, int16_t ojama)
	{
		key32_			= key;
		move16_			= move;
		score_			= score;
		remain_depth_   = remain_depth;
		generation_		= gen;
		bound_			= bound;
		player_			= player;
		time_			= remain_time;
		ojama_			= ojama;
	}

	// ����̐ݒ�p
	void setGeneration(uint8_t g) { generation_ = g; }
private:

	uint32_t key32_;
	int16_t move16_;
	int16_t score_;
	uint8_t remain_depth_;
	uint8_t generation_;
	uint8_t bound_;
	uint8_t player_;
	int16_t time_;
	int16_t ojama_;

	// ���v16�o�C�g
};

class TranspositionTable 
{
	// ���cluster�́A16�o�C�g(= sizeof(TTEntry))�~CLUSTER_SIZE = 64�o�C�g�B  
	// Cluster�́Arehash(�T���[�����[���Ȃ�����A���m�ȕ]���l���o���Ƃ��A�n�b�V���ɓo�^���Ȃ���)�̂��߂̘A������TTEntry�̉�̂��ƁB
	static const unsigned CLUSTER_SIZE = 4; // A cluster is 64 Bytes

public:
	// mem_���R���X�g���N�^�Ń[�����������Ă��Ȃ��悤�Ɍ����邪
	// �O���[�o���Ŏg�����Ƃ�z�肵�Ă���̂ŁA�����o�ϐ��͂��ׂ�0�ŏ���������Ă���B
	~TranspositionTable() { free(mem_); }

	// �u���\�𒲂ׂ�B�u���\�̃G���g���[�ւ̃|�C���^�[(TTEntry*)���Ԃ�B  
	// �G���g���[���o�^����Ă��Ȃ����NULL���Ԃ�B
	const TTEntry* probe(const LightField& self, const LightField& enemy) const;
	const TTEntry* probe(const uint64_t key) const;

	// �u���\��V�����T���̂��߂ɑ|������B(generation_��i�߂�)
	void newSearch() { ++generation_; }

	// TranspositionTable::first_entry()�́A�^����ꂽ�ǖ�(�̃n�b�V���L�[)�ɊY������  
	// �u���\���cluster�̍ŏ��̃G���g���[�ւ̃|�C���^�[��Ԃ��B  
	// �����Ƃ��ēn���ꂽkey(�n�b�V���L�[)�̉��ʃr�b�g��cluster�ւ�index�Ƃ��Ďg����B
	TTEntry* firstEntry(const uint64_t key) const;

	// TranspositionTable::set_size()�́A�u���\�̃T�C�Y��MB(���K�o�C�g)�P�ʂŐݒ肷��B  
	// �u���\��2�̗ݏ��cluster�ō\������Ă���A���ꂼ���cluster��TTEntery��CLUSTER_SIZE�Ō��܂�B  
	// (1��Cluster�́ATTEntry::CLUSTER_SIZE�~16�o�C�g)
	void setSize(size_t mbSize);

	// �u���\���tte�̃G���g���[�̐�������݂̐���(generation_)�ɂ���B
	void refresh(const TTEntry* tte) const;

	// TranspositionTable::clear()�́A�[���Œu���\�S�̂��N���A����B  
	// �e�[�u�������T�C�Y���ꂽ�Ƃ���A���[�U�[��UCI interface�o�R�Ńe�[�u���̃N���A��  
	// �v�������Ƃ��ɍs����B
	void clear();

	// �u���\�ɒl���i�[����B  
	// key : ���̋ǖʂ̃n�b�V���L�[�B
	// v : ���̋ǖʂ̒T���̌��ʓ����X�R�A
	// d : �w����𓾂��Ƃ��̎c��T���[��  
	// m : �őP��   
	// b : �]���l�̃^�C�v
	// t : ���
	// 
	
	// 	BOUND_NONE ���@�T�����Ă��Ȃ�(DEPTH_NONE)�Ƃ��ɁA�őP�肩�A�ÓI�]���X�R�A�����u���\�ɓ˂����݂����Ƃ��Ɏg���B  
	// 	BOUND_LOWER ���@fail-low  
	// 	BOUND_UPPER ��	fail-high  
	// 	BOUND_EXACT ���@���m�ȃX�R�A  
	void store(const uint64_t key, int16_t score, uint8_t depth, Move move, 
		Bound bound, uint8_t player, int16_t remain_time, int16_t ojama);

private:
	// �u���\��index��mask�p�B  
	// table_[hash_mask_ & �ǖʂ�hashkey] �����̋ǖʂ̍ŏ���entry  
	// hash_mask_ + 1 ���m�ۂ��ꂽTTEntry�̐�
	uint32_t hash_mask_;

	// �u���\�̐擪�������|�C���^�[(�m�ۂ�����������64�o�C�g�ŃA���C����������)
	TTEntry* table_;

	// �u���\�̂��߂Ɋm�ۂ������̃������ւ̃|�C���^�[�B�J������Ƃ��ɕK�v�B
	void* mem_;

	// �u���\��Entry�́A���܎g���Ă��鐢��B  
	// �����root�ǖʂ��i�ނ��ƂɃC���N�������g���Ă����B
	uint8_t generation_; 
};

extern TranspositionTable TT;


// / TranspositionTable::first_entry() returns a pointer to the first entry of
// / a cluster given a position. The lowest order bits of the key are used to
// / get the index of the cluster.

// TranspositionTable::first_entry()�́A�^����ꂽ�ǖ�(�̃n�b�V���L�[)�ɊY������
// �u���\���cluster�̍ŏ��̃G���g���[�ւ̃|�C���^�[��Ԃ��B
// �����Ƃ��ēn���ꂽkey(�n�b�V���L�[)�̉��ʃr�b�g��cluster�ւ�index�Ƃ��Ďg����B
inline TTEntry* TranspositionTable::firstEntry(const uint64_t key) const {

	return table_ + ((uint32_t)key & hash_mask_);
}

// TranspositionTable::refresh()�́ATTEntry���N���Ƃ�(���オ�i��)�̂�������邽��
// generation_�̒l���X�V����B���ʁA�u���\��hit�������ƂɌĂяo�����B
// TranspositionTable::generation_�̒l�������Ŏw�肵��TTEntry�̐���Ƃ��Đݒ肳���B
inline void TranspositionTable::refresh(const TTEntry* tte) const 
{
	const_cast<TTEntry*>(tte)->setGeneration(generation_);
}

// Size �� 2�ׂ̂���ł��邱�ƁB
template <typename T, size_t Size>
struct HashTable 
{
	HashTable() : entries_(Size, T()) {}
	T* operator [] (const uint64_t k) { return &entries_[static_cast<size_t>(k) & (Size - 1)]; }
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

	void save(const uint64_t k, const Score s, const int remain_help) 
	{
		// score��29�`31�r�b�g�ڂ�1�ɂȂ邱�Ƃ͑z�肵�Ă��Ȃ��D�܂��X�R�A���}�C�i�X�ɂȂ邱�Ƃ��z�肵�Ă��Ȃ��D
		assert(!(s & 0xe0000000) && s >= 0);
		word = static_cast<uint64_t>(k >> 32) 
			 | static_cast<uint64_t>(static_cast<int64_t>(s) << 32)
			 | static_cast<uint64_t>(remain_help) << 61;
	}

	uint64_t word;
};

struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};

// �t�B�[���h�̘A�����
struct ChainsInfo
{
	void save(const uint64_t k, const Score s, const ChainsList* pcl)
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