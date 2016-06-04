#include "chain.h"
#include "bitop.h"
#include <iostream>

// ���_����{�`i�~�i�ai�{�bi�{�ci�j}�c�@
// i�͘A�����@�A�����ƂɌv�Z���s����
// �ϐ��`�`�c�͈ȉ��ŕ\�����@
// �`���@�������Ղ�̐��@�~�P�O
// �a���@�A���{�[�i�X�@
// �b��  �R�l�N�g���@
// �c���@���������F���{�[�i�X�@

// B �A���{�[�i�X
const int ChainElement::CHAIN_BONUS[19] = { 0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512 };

// C �A���{�[�i�X
const uint8_t ChainElement::CONNECT_BONUS[8] = { 0, 2, 3, 4, 5, 6, 7, 10 };

// D ���������F���{�[�i�X
const uint8_t ChainElement::COLOR_BONUS[5] = { 0, 3, 6, 12, 24 };

ChainTable CT;

// mb_size�Ŏw�肵��MB���̃e�[�u�����m�ۂ���
void ChainTable::setSize(size_t mb_size)
{
	assert(Bitop::bsr64((mb_size << 20) / sizeof(ChainEntry)) < 32);

	uint32_t size = CLUSTER_SIZE << Bitop::bsr64((mb_size << 20) / sizeof(ChainEntry[CLUSTER_SIZE]));

	if (hash_mask_ == size - CLUSTER_SIZE)
		return;

	hash_mask_ = size - CLUSTER_SIZE;

	mem_ = calloc(size * sizeof(ChainEntry) + CACHE_LINE_SIZE - 1, 1);

	if (!mem_)
	{
		std::cerr << "Failed to allocate " << mb_size << "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}

	// 64�̔{���̃A�h���X��table�̎n�܂�ɂ���
	table_ = (ChainEntry*)((uintptr_t(mem_) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

// �u���\��0�N���A����D
void ChainTable::clear()
{
	std::memset(table_, 0, (hash_mask_ + CLUSTER_SIZE) * sizeof(ChainEntry));
}

// �u���\����key�Ɉ�v����G���g���[�ւ̃|�C���^��Ԃ��D
const ChainEntry* ChainTable::probe(const uint64_t key) const 
{
	const ChainEntry* chain_entry = firstEntry(key);
	const uint32_t key32 = key >> 32;

	// �L�[�Ɉ�v������Ԃ��D
	for (int i = 0; i < CLUSTER_SIZE; ++i, ++chain_entry)
	{
		if (chain_entry->key() == key32)
			return chain_entry;
	}

	return nullptr;
}

// �u���\�ɃG���g���[��o�^����D
void ChainTable::store(const uint64_t key, const uint64_t after_key, const ChainElement& ce, const Bitboard& bb)
{
	ChainEntry *chain_entry, *replace;

	chain_entry = replace = firstEntry(key);

	// ���̋ǖʂ̃n�b�V���L�[�̏��32�r�b�g��o�^����D
	const uint32_t key32 = key >> 32;

	for (int i = 0; i < CLUSTER_SIZE; ++i, ++chain_entry)
	{
		// ��������̓L�[����v
		if (!chain_entry->key() || chain_entry->key() == key32)
		{
			// ��̃G���g���[�����������̂ŁC�������g���D
			replace = chain_entry;
			break;
		}

		// �o�^����Ă���ǖʂ��A�������傫���Ȃ�C�����炭���̋ǖʂ̂ق����厖�Ȃ̂��낤�D
		//if (ce.chain() > chain_entry->chainElement()->chain())
			//replace = chain_entry;

		// ��ɏ㏑������B
		replace = chain_entry;
	}
	replace->save(key32, after_key, ce, bb);
}