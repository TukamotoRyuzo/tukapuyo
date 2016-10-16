#include"tt.h"
#include"bitop.h"
#include <iostream>

// �u���\�̓O���[�o���ɔz�u
TranspositionTable TT; 
EvaluateHashTable ET;
ChainsInfoTable CIT;

// TranspositionTable::set_size()�́A�u���\�̃T�C�Y��MB(���K�o�C�g)�P�ʂŐݒ肷��B
void TranspositionTable::resize(size_t mb_size)
{
	size_t new_cluster_count = size_t(1) << Bitop::bsr64((mb_size * 1024 * 1024) / sizeof(Cluster));

	// ���݊m�ے��̒u���\�p�̃������Ɠ������Ȃ�΍Ċm�ۂ͍s��Ȃ�
	if (new_cluster_count == cluster_count_)
		return;

	cluster_count_ = new_cluster_count;

	free(mem_);

	mem_ = calloc(cluster_count_ * sizeof(Cluster) + CACHE_LINE_SIZE - 1, 1);

	if (!mem_)
	{
		std::cerr << "Failed to allocate " << mb_size << "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}

	// mem_����64�o�C�g�ŃA���C�����ꂽ�A�h���X�𓾂āA�����table�Ƃ��Ďg���B
	table_ = (Cluster*)((uintptr_t(mem_) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

// �u���\�𒲂ׂ�B�u���\�̃G���g���[�ւ̃|�C���^�[(TTEntry*)���Ԃ�B
// �G���g���[���o�^����Ă��Ȃ����NULL���Ԃ�
template bool TranspositionTable::probe<true >(const LightField* self, const LightField* enemy, TTEntry* &ptt) const;
template bool TranspositionTable::probe<false>(const LightField* self, const LightField* enemy, TTEntry* &ptt) const;

template <bool OnePlayer>
bool TranspositionTable::probe(const LightField* self, const LightField* enemy, TTEntry* &ptt) const
{
	Key key = OnePlayer ? self->key() : self->key() ^ enemy->key();

	// �ŏ��̃G���g���[���擾
	TTEntry* const tte = firstEntry(key);
	uint32_t key32 = key >> 32;

	for (int i = 0; i < CLUSTER_SIZE; ++i)
	{
		// �󂩁A�����ǖʂ���������
		if (!tte[i].key32_ || (tte[i].key32_ == key32))
		{
			if (OnePlayer
				|| (tte->player() == self->player() // ��Ղ���v
					&& tte->ojama() == self->ojama() - enemy->ojama())) // ������܂Ղ�̐�����v
			{
				if (tte[i].generation_ != generation8_ && tte[i].key32_)
					tte[i].generation_ = generation8_; // Refresh

				ptt = &tte[i];
				return (bool)tte[i].key32_;
			}
		}
	}

	// ������Ȃ�������Areplace�ł���|�C���^��Ԃ��B
	TTEntry* replace = tte;

	for (int i = 1; i < CLUSTER_SIZE; ++i)
		if (replace->depth_ > tte[i].depth_) // ��Ԏc��[���̏��Ȃ��ǖʂ�replace���Ƃ���
			replace = &tte[i];

	ptt = replace;
	return false;
}