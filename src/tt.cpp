#include"tt.h"
#include"bitop.h"
#include <iostream>

// �u���\�̓O���[�o���ɔz�u
TranspositionTable TT; 
EvaluateHashTable ET;
ChainsInfoTable CIT;

// TranspositionTable::set_size()�́A�u���\�̃T�C�Y��MB(���K�o�C�g)�P�ʂŐݒ肷��B
// �u���\��2�̗ݏ��cluster�ō\������Ă���A���ꂼ���cluster��TTEntery��CLUSTER_SIZE�Ō��܂�B
// (1��Cluster�́ATTEntry::CLUSTER_SIZE�~16�o�C�g)
// mbSize��CLUSTER_SIZE[GB]�܂ł̂悤���BCLUSTER_SIZE = 16������16GB�܂ŁB

void TranspositionTable::setSize(size_t mbSize)
{

	// MB�P�ʂŎw�肵�Ă���̂ō�20��V�t�g (1MB = 2��20�悾����)  
	// TTEntry�̐���32bit��index�ł���͈͂𒴂��Ă���Ƃ܂����B  
	// TTEntry��128bit(16�o�C�g)�Ȃ̂�64GB���u���\�p�̃T�C�Y�̏���B  
	// ToDo : ����assert�{���ɗv��̂��H64GB�ȏ�m�ۂł��Ă������悤�Ɏv���̂����c�B
	assert(Bitop::bsr64((mbSize << 20) / sizeof(TTEntry)) < 32);

	// mbSize[MB] / cluster�̃T�C�Y�̂����A�ŏ�ʂ�1�ɂȂ��Ă���bit�ȊO�͒[���Ƃ݂Ȃ���0�ɂ���������  
	// cluster���m�ۂ���B
	uint32_t size = CLUSTER_SIZE << Bitop::bsr64((mbSize << 20) / sizeof(TTEntry[CLUSTER_SIZE]));

	// ���݊m�ے��̒u���\�p�̃������Ɠ������Ȃ�΍Ċm�ۂ͍s��Ȃ�
	if (hash_mask_ == size - CLUSTER_SIZE)
		return;

	// hash_mask_�́Asize��CLUSTER_SIZE�~010....0b�݂����Ȑ��Ȃ̂ŁA��������CLUSTER_SIZE�������Z�����  
	// 0011....1b�݂����Ȑ��ɂȂ�B�����mask�Ƃ��Ďg���B  
	// �t�ɁA�m�ۂ��Ă���TTEntry�̐��́Ahash_mask_ + CLUSTER_SIZE�ł���B
	hash_mask_ = size - CLUSTER_SIZE;

	// �O��m�ۂ��Ă������������J��
	free(mem_);

	// �m�ۂ��Ȃ����Bcalloc���g���Ă���̂̓[���N���A���邽�߁B  
	// ���ƁACACHE_LINE_SIZE�́A64�B�]���Ɋm�ۂ��āA64�ŃA���C�����ꂽ�A�h���X�𓾂邽�߁B
	mem_ = calloc(size * sizeof(TTEntry) + CACHE_LINE_SIZE - 1, 1);

	if (!mem_)
	{
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}

	// mem_����64�o�C�g�ŃA���C�����ꂽ�A�h���X�𓾂āA�����table�Ƃ��Ďg���B
	table_ = (TTEntry*)((uintptr_t(mem_) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

// TranspositionTable::clear()�́A�[���Œu���\�S�̂��N���A����B
// �e�[�u�������T�C�Y���ꂽ�Ƃ���A���[�U�[��UCI interface�o�R�Ńe�[�u���̃N���A��
// �v�������Ƃ��ɍs����B
void TranspositionTable::clear()
{
	// hash_mask_ + CLUSTER_SIZE�ɂȂ��Ă��闝�R��TranspositionTable::set_size�ɂ��������ǂނ��ƁB 
	std::memset(table_, 0, (hash_mask_ + CLUSTER_SIZE) * sizeof(TTEntry));
	generation_ = 0;
}
// �u���\�𒲂ׂ�B�u���\�̃G���g���[�ւ̃|�C���^�[(TTEntry*)���Ԃ�B
// �G���g���[���o�^����Ă��Ȃ����NULL���Ԃ�
const TTEntry* TranspositionTable::probe(const Key key) const
{
	// �ŏ��̃G���g���[���擾
	const TTEntry* tte = firstEntry(key);

	// �u���\�ɓo�^����Ă���hash�̂ƈ�v����̂����m�F����B
	uint32_t key32 = key >> 32;

	// CLUSTER_SIZE������rehash�����Ɖ��߁B
	for (unsigned i = 0; i < CLUSTER_SIZE; ++i, ++tte)
	{
		// �n�b�V���L�[����v
		if (tte->key() == key32)
			return tte;
	}

	// ���������Ȃ炻���Ԃ��B�����Ȃ���null 
	return nullptr;
}

// �u���\�𒲂ׂ�B�u���\�̃G���g���[�ւ̃|�C���^�[(TTEntry*)���Ԃ�B
// �G���g���[���o�^����Ă��Ȃ����NULL���Ԃ�
const TTEntry* TranspositionTable::probe(const LightField& self, const LightField& enemy) const
{
	Key key = self.key() ^ enemy.key();

	// �ŏ��̃G���g���[���擾
	const TTEntry* tte = firstEntry(key);

	// �u���\�ɓo�^����Ă���hash�̂ƈ�v����̂����m�F����B
	uint32_t key32 = key >> 32;

	// CLUSTER_SIZE������rehash�����Ɖ��߁B
	for (unsigned i = 0; i < CLUSTER_SIZE; ++i, ++tte)
	{
		// �n�b�V���L�[����v
		if (tte->key() == key32)
		{
			// ��Ղ���v
			if (tte->player() == self.player())
			{
				// ������܂Ղ�̐�����v
				if (tte->ojama() == self.ojama() - enemy.ojama())
				{
					return tte;
				}
			}
		}
	}

	// ���������Ȃ炻���Ԃ��B�����Ȃ���null 
	return nullptr;
}



// TranspositionTable::store()�͌��݂̋ǖʂ́A�ǖʃn�b�V���L�[�Ɖ��l�̂������V�����G���g���[�ɏ������ށB
// �ǖʃn�b�V���L�[�̉��ʂ̃I�[�_�[��bit�́A�ǂ���cluster���i�[���ׂ��������肷��̂Ɏg����B
// �V�����G���g���[���������܂��Ƃ��ɁA���p�ł����̃G���g���[��cluster��ɂȂ����
// �����Ƃ����l�̒Ⴂ�G���g���[��u��������B
// �Q��TTEntry t1,t2������Ƃ��āAt1�����݂̒T���ŁAt2���ȑO�̒T��(���オ�Â�)���Ƃ��A
// t1�̒T���[���̂ق���t2�̒T���[�����[�����Ƃ�����ꍇ�́At1�̂ق���t2��艿�l������ƍl������B

// �u���\�ɒl���i�[����B
// key : ���̋ǖʂ̃n�b�V���L�[�B
// v : ���̋ǖʂ̒T���̌��ʓ����X�R�A
// b : ���̃X�R�A�̐����B
// 	BOUND_NONE ���@�T�����Ă��Ȃ�(DEPTH_NONE)�Ƃ��ɁA�őP�肩�A�ÓI�]���X�R�A�����u���\�ɓ˂����݂����Ƃ��Ɏg���B
// 	BOUND_LOWER ���@fail-low
// 	BOUND_UPPER ��	fail-high
// 	BOUND_EXACT ���@���m�ȃX�R�A
// d : ���̃X�R�A�E�w����𓾂��Ƃ��̎c��T���[��
// m : �őP��
// statV : �ÓI�]��(���̋ǖʂŕ]���֐����Ăяo���ē����l)

void TranspositionTable::store(const Key key, int16_t score, uint8_t depth, Move move, Bound bound, uint8_t player,
							   int16_t remain_time, int16_t ojama)
{
	TTEntry *tte, *replace;

	// �ǖʃn�b�V���L�[�̏��32bit��cluster(���u���\�̃G���g���[)�̂Ȃ��ŗp����B  
	// �܂�����bit��cluster�ւ�index�Ƃ��ėp����
	uint32_t key32 = key >> 32;

	// cluster�̐擪��TTEntry�𓾂�
	tte = replace = firstEntry(key);

	// cluster�̂��ꂼ��̗v�f�ɂ���..
	for (unsigned i = 0; i < CLUSTER_SIZE; ++i, ++tte)
	{
		// ���������key����v���āA��Ԃ���v����.
		if (!tte->key() || tte->key() == key32 && tte->player() == player)
		{
			// ���܁A�w���肪�Ȃ��Ȃ�Ύw��������ɂ���ׂ����ɕۑ����Ă����B      
			if (move.isNone())
				move = tte->move();

			// �Ƃ������A�󂩁A���̋ǖʗp�̌Â��G���g���[�����������̂ł������g���B
			replace = tte;
			break;
		}

		// Implement replace strategy
		 // ��ԉ��l�̒Ⴂ�Ƃ���ƒu������     
		// replace��Cluster�̂Ȃ��ł��܈�ԉ��l���Ⴂ�Ƃ����TTEntry    
		// ����A�s�xreplace->XXX�Ɣ�r�ɂȂ��Ă���̂������C�����������A�Y��ȃR�[�h�Ȃ̂ł܂������̂��c�B     
		// ���ܒu���\���entry��replace�|�C���^�[���w���Ă�����̂Ƃ��āA    
		// replace��generation���A���݂�generation�ƈ�v����Ȃ�� +2    
		// tte��generation�����݂�generation�A��������tte��BOUND_EXACT�Ȃ�-2    
		// tte�̒T���[���̂ق���replace�̒T���[�����󂢂Ȃ� +1

		int c1 = (replace->generation() == generation_ ?  2 : 0);
		int c2 = (tte->generation() == generation_ || tte->bound() == BOUND_EXACT ? -2 : 0);
		int c3 = (tte->remainDepth() < replace->remainDepth() ?  1 : 0);

		if (c1 + c2 + c3 > 0)
			replace = tte;

		// �P�[�X1) generation��replace,tte�Ƃ��ɍŐV�ł���ꍇ�B    
		// replace�̂ق����T���[�����������󂢂Ȃ�replace(�ŏ��Ɍ��������ق�)��u��������B    
		// (�Ȃ�ׂ��擪�ɋ߂��Ƃ����u���������ق���probe�������I�邽��)    
		
		// �P�[�X2) generation��replace�͍ŐV�����Atte��BOUND_EXACT�ł���ꍇ�B    
		// replace��u��������

		// ToDo : ���̃��W�b�N�ǂ��Ȃ�H    
		// �T���[�����[���ق��Ƃ����d�݂��Ⴂ�̂ŁA�T���[�����[���낤�Ƃ��A���オ�Â���    
		// �㏑������Ă��܂��B�T���[�����[���Ƃ��̏d�݂����������傫�����āA�܂��A    
		// ���㍷2�܂ł́A���_����Ȃǂ��ăo�����X���Ƃ����ق��������̂ł́c�B
	}

	// �u���Ώۂ����܂����̂ł����ɑ΂��Ēl���㏑�����Ă��܂��B  
	// ����generation�́A����TranspositionTable���ێ����Ă��郁���o�[�ϐ��B

	replace->save(key32, move.get(), score, depth, generation_, bound, player, remain_time, ojama);
}