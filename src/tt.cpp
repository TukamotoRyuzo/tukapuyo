#include"tt.h"
#include"bitop.h"
#include <iostream>


// 置換表はグローバルに配置
TranspositionTable TT; 
EvaluateHashTable ET;
ChainsInfoTable CIT;
// TranspositionTable::set_size()は、置換表のサイズをMB(メガバイト)単位で設定する。
// 置換表は2の累乗のclusterで構成されており、それぞれのclusterはTTEnteryのCLUSTER_SIZEで決まる。
// (1つのClusterは、TTEntry::CLUSTER_SIZE×16バイト)
// mbSizeはCLUSTER_SIZE[GB]までのようだ。CLUSTER_SIZE = 16だから16GBまで。

void TranspositionTable::setSize(size_t mbSize)
{

	// MB単位で指定してあるので左20回シフト (1MB = 2の20乗だから)  
	// TTEntryの数が32bitでindexできる範囲を超えているとまずい。  
	// TTEntryが128bit(16バイト)なので64GBが置換表用のサイズの上限。  
	// ToDo : このassert本当に要るのか？64GB以上確保できてもいいように思うのだが…。
	assert(Bitop::bsr64((mbSize << 20) / sizeof(TTEntry)) < 32);

	// mbSize[MB] / clusterのサイズのうち、最上位の1になっているbit以外は端数とみなして0にした個数だけ  
	// clusterを確保する。
	uint32_t size = CLUSTER_SIZE << Bitop::bsr64((mbSize << 20) / sizeof(TTEntry[CLUSTER_SIZE]));

	// 現在確保中の置換表用のメモリと等しいならば再確保は行わない
	if (hash_mask_ == size - CLUSTER_SIZE)
		return;

	// hash_mask_は、sizeがCLUSTER_SIZE×010....0bみたいな数なので、ここからCLUSTER_SIZEを引き算すると  
	// 0011....1bみたいな数になる。これをmaskとして使う。  
	// 逆に、確保してあるTTEntryの数は、hash_mask_ + CLUSTER_SIZEである。
	hash_mask_ = size - CLUSTER_SIZE;

	// 前回確保していたメモリを開放
	free(mem_);

	// 確保しなおす。callocを使ってあるのはゼロクリアするため。  
	// あと、CACHE_LINE_SIZEは、64。余分に確保して、64でアラインされたアドレスを得るため。
	mem_ = calloc(size * sizeof(TTEntry) + CACHE_LINE_SIZE - 1, 1);

	if (!mem_)
	{
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}

	// mem_から64バイトでアラインされたアドレスを得て、それをtableとして使う。
	table_ = (TTEntry*)((uintptr_t(mem_) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

// TranspositionTable::clear()は、ゼロで置換表全体をクリアする。
// テーブルがリサイズされたときや、ユーザーがUCI interface経由でテーブルのクリアを
// 要求したときに行われる。
void TranspositionTable::clear()
{
	// hash_mask_ + CLUSTER_SIZEになっている理由はTranspositionTable::set_sizeにある説明を読むこと。 
	std::memset(table_, 0, (hash_mask_ + CLUSTER_SIZE) * sizeof(TTEntry));
	generation_ = 0;
}
// 置換表を調べる。置換表のエントリーへのポインター(TTEntry*)が返る。
// エントリーが登録されていなければNULLが返る
const TTEntry* TranspositionTable::probe(const uint64_t key) const
{
	// 最初のエントリーを取得
	const TTEntry* tte = firstEntry(key);

	// 置換表に登録されているhashのと一致するのかを確認する。
	uint32_t key32 = key >> 32;

	// CLUSTER_SIZE分だけrehashされると解釈。
	for (unsigned i = 0; i < CLUSTER_SIZE; ++i, ++tte)
	{
		// ハッシュキーが一致
		if (tte->key() == key32)
			return tte;
	}

	// 見つかったならそれを返す。さもなくばnull 
	return nullptr;
}

// 置換表を調べる。置換表のエントリーへのポインター(TTEntry*)が返る。
// エントリーが登録されていなければNULLが返る
const TTEntry* TranspositionTable::probe(const LightField& self, const LightField& enemy) const
{
	uint64_t key = self.key() ^ enemy.key();

	// 最初のエントリーを取得
	const TTEntry* tte = firstEntry(key);

	// 置換表に登録されているhashのと一致するのかを確認する。
	uint32_t key32 = key >> 32;

	// CLUSTER_SIZE分だけrehashされると解釈。
	for (unsigned i = 0; i < CLUSTER_SIZE; ++i, ++tte)
	{
		// ハッシュキーが一致
		if (tte->key() == key32)
		{
			// 手盤が一致
			if (tte->player() == self.player())
			{
				// おじゃまぷよの数が一致
				if (tte->ojama() == self.ojama() - enemy.ojama())
				{
					return tte;
				}
			}
		}
	}

	// 見つかったならそれを返す。さもなくばnull 
	return nullptr;
}



// TranspositionTable::store()は現在の局面の、局面ハッシュキーと価値のある情報を新しいエントリーに書き込む。
// 局面ハッシュキーの下位のオーダーのbitは、どこにclusterを格納すべきかを決定するのに使われる。
// 新しいエントリーが書き込まれるときに、利用できる空のエントリーがcluster上になければ
// もっとも価値の低いエントリーを置き換える。
// ２つのTTEntry t1,t2があるとして、t1が現在の探索で、t2が以前の探索(世代が古い)だとか、
// t1の探索深さのほうがt2の探索深さより深いだとかする場合は、t1のほうがt2より価値があると考えられる。

// 置換表に値を格納する。
// key : この局面のハッシュキー。
// v : この局面の探索の結果得たスコア
// b : このスコアの性質。
// 	BOUND_NONE →　探索していない(DEPTH_NONE)ときに、最善手か、静的評価スコアだけ置換表に突っ込みたいときに使う。
// 	BOUND_LOWER →　fail-low
// 	BOUND_UPPER →	fail-high
// 	BOUND_EXACT →　正確なスコア
// d : このスコア・指し手を得たときの残り探索深さ
// m : 最善手
// statV : 静的評価(この局面で評価関数を呼び出して得た値)

void TranspositionTable::store(const uint64_t key, int16_t score, uint8_t depth, Move move, Bound bound, uint8_t player,
							   int16_t remain_time, int16_t ojama)
{
	TTEntry *tte, *replace;

	// 局面ハッシュキーの上位32bitはcluster(≒置換表のエントリー)のなかで用いる。  
	// また下位bitをclusterへのindexとして用いる
	uint32_t key32 = key >> 32;

	// clusterの先頭のTTEntryを得る
	tte = replace = firstEntry(key);

	// clusterのそれぞれの要素について..
	for (unsigned i = 0; i < CLUSTER_SIZE; ++i, ++tte)
	{
		// 空もしくはkeyが一致して、手番も一致する.
		if (!tte->key() || tte->key() == key32 && tte->player() == player)
		{
			// いま、指し手がないならば指し手を何にせよ潰さずに保存しておく。      
			if (move.isNone())
				move = tte->move();

			// ともかく、空か、この局面用の古いエントリーが見つかったのでそこを使う。
			replace = tte;
			break;
		}

		// Implement replace strategy
		 // 一番価値の低いところと置換する     
		// replaceはClusterのなかでいま一番価値が低いとされるTTEntry    
		// これ、都度replace->XXXと比較になっているのが少し気持ち悪いが、綺麗なコードなのでまあいいのか…。     
		// いま置換予定のentryをreplaceポインターが指しているものとして、    
		// replaceのgenerationが、現在のgenerationと一致するならば +2    
		// tteのgenerationが現在のgeneration、もしくはtteがBOUND_EXACTなら-2    
		// tteの探索深さのほうがreplaceの探索深さより浅いなら +1

		int c1 = (replace->generation() == generation_ ?  2 : 0);
		int c2 = (tte->generation() == generation_ || tte->bound() == BOUND_EXACT ? -2 : 0);
		int c3 = (tte->remainDepth() < replace->remainDepth() ?  1 : 0);

		if (c1 + c2 + c3 > 0)
			replace = tte;

		// ケース1) generationがreplace,tteともに最新である場合。    
		// replaceのほうが探索深さが同じか浅いならreplace(最初に見つかったほう)を置き換える。    
		// (なるべく先頭に近いところを置き換えたほうがprobeが早く終るため)    
		
		// ケース2) generationがreplaceは最新だが、tteがBOUND_EXACTである場合。    
		// replaceを置き換える

		// ToDo : このロジックどうなん？    
		// 探索深さが深いほうという重みが低いので、探索深さが深かろうとも、世代が古いと    
		// 上書きされてしまう。探索深さが深いときの重みをもう少し大きくして、また、    
		// 世代差2までは、加点するなどしてバランスをとったほうがいいのでは…。
	}

	// 置換対象が決まったのでそこに対して値を上書きしてしまう。  
	// このgenerationは、このTranspositionTableが保持しているメンバー変数。

	replace->save(key32, move.get(), score, depth, generation_, bound, player, remain_time, ojama);
}