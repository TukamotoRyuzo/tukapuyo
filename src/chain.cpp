#include "chain.h"
#include "bitop.h"
#include <iostream>

// 得点＝Σ{Ａi×（Ｂi＋Ｃi＋Ｄi）}…①
// iは連鎖数　連鎖ごとに計算が行われる
// 変数Ａ～Ｄは以下で表される　
// Ａ＝　消したぷよの数　×１０
// Ｂ＝　連鎖ボーナス　
// Ｃ＝  コネクト数　
// Ｄ＝　同時消し色数ボーナス　

// B 連鎖ボーナス
const int ChainElement::CHAIN_BONUS[19] = { 0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512 };

// C 連結ボーナス
const uint8_t ChainElement::CONNECT_BONUS[8] = { 0, 2, 3, 4, 5, 6, 7, 10 };

// D 同時消し色数ボーナス
const uint8_t ChainElement::COLOR_BONUS[5] = { 0, 3, 6, 12, 24 };

ChainTable CT;

// mb_sizeで指定したMB数のテーブルを確保する
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

	// 64の倍数のアドレスをtableの始まりにする
	table_ = (ChainEntry*)((uintptr_t(mem_) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

// 置換表を0クリアする．
void ChainTable::clear()
{
	std::memset(table_, 0, (hash_mask_ + CLUSTER_SIZE) * sizeof(ChainEntry));
}

// 置換表からkeyに一致するエントリーへのポインタを返す．
const ChainEntry* ChainTable::probe(const Key key) const 
{
	const ChainEntry* chain_entry = firstEntry(key);
	const uint32_t key32 = key >> 32;

	// キーに一致したら返す．
	for (int i = 0; i < CLUSTER_SIZE; ++i, ++chain_entry)
	{
		if (chain_entry->key() == key32)
			return chain_entry;
	}

	return nullptr;
}

// 置換表にエントリーを登録する．
void ChainTable::store(const Key key, const Key after_key, const ChainElement& ce, const Bitboard& bb)
{
	ChainEntry *chain_entry, *replace;

	chain_entry = replace = firstEntry(key);

	// この局面のハッシュキーの上位32ビットを登録する．
	const uint32_t key32 = key >> 32;

	for (int i = 0; i < CLUSTER_SIZE; ++i, ++chain_entry)
	{
		// 空もしくはキーが一致
		if (!chain_entry->key() || chain_entry->key() == key32)
		{
			// 空のエントリーが見つかったので，そこを使う．
			replace = chain_entry;
			break;
		}

		// 登録されている局面より連鎖数が大きいなら，おそらく今の局面のほうが大事なのだろう．
		//if (ce.chain() > chain_entry->chainElement()->chain())
			//replace = chain_entry;

		// 常に上書きする。
		replace = chain_entry;
	}
	replace->save(key32, after_key, ce, bb);
}