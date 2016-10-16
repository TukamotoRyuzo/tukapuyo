#include"tt.h"
#include"bitop.h"
#include <iostream>

// 置換表はグローバルに配置
TranspositionTable TT; 
EvaluateHashTable ET;
ChainsInfoTable CIT;

// TranspositionTable::set_size()は、置換表のサイズをMB(メガバイト)単位で設定する。
void TranspositionTable::resize(size_t mb_size)
{
	size_t new_cluster_count = size_t(1) << Bitop::bsr64((mb_size * 1024 * 1024) / sizeof(Cluster));

	// 現在確保中の置換表用のメモリと等しいならば再確保は行わない
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

	// mem_から64バイトでアラインされたアドレスを得て、それをtableとして使う。
	table_ = (Cluster*)((uintptr_t(mem_) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

// 置換表を調べる。置換表のエントリーへのポインター(TTEntry*)が返る。
// エントリーが登録されていなければNULLが返る
template bool TranspositionTable::probe<true >(const LightField* self, const LightField* enemy, TTEntry* &ptt) const;
template bool TranspositionTable::probe<false>(const LightField* self, const LightField* enemy, TTEntry* &ptt) const;

template <bool OnePlayer>
bool TranspositionTable::probe(const LightField* self, const LightField* enemy, TTEntry* &ptt) const
{
	Key key = OnePlayer ? self->key() : self->key() ^ enemy->key();

	// 最初のエントリーを取得
	TTEntry* const tte = firstEntry(key);
	uint32_t key32 = key >> 32;

	for (int i = 0; i < CLUSTER_SIZE; ++i)
	{
		// 空か、同じ局面が見つかった
		if (!tte[i].key32_ || (tte[i].key32_ == key32))
		{
			if (OnePlayer
				|| (tte->player() == self->player() // 手盤が一致
					&& tte->ojama() == self->ojama() - enemy->ojama())) // おじゃまぷよの数が一致
			{
				if (tte[i].generation_ != generation8_ && tte[i].key32_)
					tte[i].generation_ = generation8_; // Refresh

				ptt = &tte[i];
				return (bool)tte[i].key32_;
			}
		}
	}

	// 見つからなかったら、replaceできるポインタを返す。
	TTEntry* replace = tte;

	for (int i = 1; i < CLUSTER_SIZE; ++i)
		if (replace->depth_ > tte[i].depth_) // 一番残り深さの少ない局面をreplace候補とする
			replace = &tte[i];

	ptt = replace;
	return false;
}