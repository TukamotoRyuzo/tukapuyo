#pragma once
#include"platform.h"
#include"const.h"
#include"field.h"
#include<cstdlib>

struct TTEntry
{
	
	// このクラスのインスタンスはcallocで確保されるので、コンストラクタが呼び出されることはない。  
	// TTEntry*を得て、そこに対してsave()などで値を書き込んだりする

	// 局面を識別するために必要な情報は
	// 1.自分のフィールドのハッシュキー
	// 2.相手のフィールドのハッシュキー
	// 3.現在のツモ番号(最大127: 7bit)
	// 4.自分の残り時間(最大1337 : 大体11bit)
	// 5.予告おじゃまぷよの数(最大2000: 大体11bit)
	// 6.自分、相手が全消しを持っているかどうか

	// である。しかしこれらのハッシュシードを用意しハッシュキーに加えるのはかなり手間。
	// ハッシュキーには1,2,3,6を考慮したシード値をXORしていく(そうしないと違う局面を同じ局面として登録する可能性が高くなる)
	// 局面のハッシュキー以外はエントリーに登録することにする．
	// 局面のハッシュキーは自分^相手である

	// 以下、getter
	// 意味は↓のprivateを参照しろ

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

	// 世代の設定用
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

	// 合計16バイト
};

class TranspositionTable 
{
	// 一つのclusterは、16バイト(= sizeof(TTEntry))×CLUSTER_SIZE = 64バイト。  
	// Clusterは、rehash(探索深さが深くなったり、正確な評価値が出たとき、ハッシュに登録しなおす)のための連続したTTEntryの塊のこと。
	static const unsigned CLUSTER_SIZE = 4; // A cluster is 64 Bytes

public:
	// mem_をコンストラクタでゼロ初期化していないように見えるが
	// グローバルで使うことを想定しているので、メンバ変数はすべて0で初期化されている。
	~TranspositionTable() { free(mem_); }

	// 置換表を調べる。置換表のエントリーへのポインター(TTEntry*)が返る。  
	// エントリーが登録されていなければNULLが返る。
	const TTEntry* probe(const LightField& self, const LightField& enemy) const;
	const TTEntry* probe(const uint64_t key) const;

	// 置換表を新しい探索のために掃除する。(generation_を進める)
	void newSearch() { ++generation_; }

	// TranspositionTable::first_entry()は、与えられた局面(のハッシュキー)に該当する  
	// 置換表上のclusterの最初のエントリーへのポインターを返す。  
	// 引数として渡されたkey(ハッシュキー)の下位ビットがclusterへのindexとして使われる。
	TTEntry* firstEntry(const uint64_t key) const;

	// TranspositionTable::set_size()は、置換表のサイズをMB(メガバイト)単位で設定する。  
	// 置換表は2の累乗のclusterで構成されており、それぞれのclusterはTTEnteryのCLUSTER_SIZEで決まる。  
	// (1つのClusterは、TTEntry::CLUSTER_SIZE×16バイト)
	void setSize(size_t mbSize);

	// 置換表上のtteのエントリーの世代を現在の世代(generation_)にする。
	void refresh(const TTEntry* tte) const;

	// TranspositionTable::clear()は、ゼロで置換表全体をクリアする。  
	// テーブルがリサイズされたときや、ユーザーがUCI interface経由でテーブルのクリアを  
	// 要求したときに行われる。
	void clear();

	// 置換表に値を格納する。  
	// key : この局面のハッシュキー。
	// v : この局面の探索の結果得たスコア
	// d : 指し手を得たときの残り探索深さ  
	// m : 最善手   
	// b : 評価値のタイプ
	// t : 手番
	// 
	
	// 	BOUND_NONE →　探索していない(DEPTH_NONE)ときに、最善手か、静的評価スコアだけ置換表に突っ込みたいときに使う。  
	// 	BOUND_LOWER →　fail-low  
	// 	BOUND_UPPER →	fail-high  
	// 	BOUND_EXACT →　正確なスコア  
	void store(const uint64_t key, int16_t score, uint8_t depth, Move move, 
		Bound bound, uint8_t player, int16_t remain_time, int16_t ojama);

private:
	// 置換表のindexのmask用。  
	// table_[hash_mask_ & 局面のhashkey] がその局面の最初のentry  
	// hash_mask_ + 1 が確保されたTTEntryの数
	uint32_t hash_mask_;

	// 置換表の先頭を示すポインター(確保したメモリを64バイトでアラインしたもの)
	TTEntry* table_;

	// 置換表のために確保した生のメモリへのポインター。開放するときに必要。
	void* mem_;

	// 置換表のEntryの、いま使っている世代。  
	// これをroot局面が進むごとにインクリメントしていく。
	uint8_t generation_; 
};

extern TranspositionTable TT;


// / TranspositionTable::first_entry() returns a pointer to the first entry of
// / a cluster given a position. The lowest order bits of the key are used to
// / get the index of the cluster.

// TranspositionTable::first_entry()は、与えられた局面(のハッシュキー)に該当する
// 置換表上のclusterの最初のエントリーへのポインターを返す。
// 引数として渡されたkey(ハッシュキー)の下位ビットがclusterへのindexとして使われる。
inline TTEntry* TranspositionTable::firstEntry(const uint64_t key) const {

	return table_ + ((uint32_t)key & hash_mask_);
}

// TranspositionTable::refresh()は、TTEntryが年をとる(世代が進む)のを回避するため
// generation_の値を更新する。普通、置換表にhitしたあとに呼び出される。
// TranspositionTable::generation_の値が引数で指定したTTEntryの世代として設定される。
inline void TranspositionTable::refresh(const TTEntry* tte) const 
{
	const_cast<TTEntry*>(tte)->setGeneration(generation_);
}

// Size は 2のべき乗であること。
template <typename T, size_t Size>
struct HashTable 
{
	HashTable() : entries_(Size, T()) {}
	T* operator [] (const uint64_t k) { return &entries_[static_cast<size_t>(k) & (Size - 1)]; }
	void clear() { std::fill(std::begin(entries_), std::end(entries_), T()); }
	// Size が 2のべき乗であることのチェック
	static_assert((Size & (Size - 1)) == 0, "");

private:
	std::vector<T> entries_;
};

const size_t EvaluateTableSize = 0x1000000; // 134MB
const size_t ChainsInfoTableSize = 0x1000000; // 134MB
// 64bit 変数1つなのは理由があって、
// データを取得している最中に他のスレッドから書き換えられることが無くなるから。
// lockless hash と呼ばれる。
// 128bit とかだったら、64bitずつ2回データを取得しないといけないので、
// key と score が対応しない可能性がある。
// transposition table は正にその問題を抱えているが、
// 静的評価値のように差分評価をする訳ではないので、問題になることは少ない。
// 64bitに収まらない場合や、transposition table なども安全に扱いたいなら、
// lockする、SSEやAVX命令を使う、チェックサムを持たせる、key を複数の変数に分けて保持するなどの方法がある。
// 32bit OS 場合、64bit 変数を 32bitずつ2回データ取得するので、下位32bitを上位32bitでxorして
// データ取得時に壊れていないか確認する。
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
		// scoreの29～31ビット目が1になることは想定していない．またスコアがマイナスになることも想定していない．
		assert(!(s & 0xe0000000) && s >= 0);
		word = static_cast<uint64_t>(k >> 32) 
			 | static_cast<uint64_t>(static_cast<int64_t>(s) << 32)
			 | static_cast<uint64_t>(remain_help) << 61;
	}

	uint64_t word;
};

struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};

// フィールドの連鎖情報
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