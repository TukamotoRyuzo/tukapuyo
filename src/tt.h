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
			|| d >= depth_
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

	// 世代の設定用
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

	// 合計16バイト
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
	// 置換表のindexのmask用。  
	size_t cluster_count_;

	// 置換表の先頭を示すポインター(確保したメモリを64バイトでアラインしたもの)
	Cluster* table_;

	// 置換表のために確保した生のメモリへのポインター。開放するときに必要。
	void* mem_;

	// 置換表のEntryの、いま使っている世代。  
	// これをroot局面が進むごとにインクリメントしていく。
	uint8_t generation8_; 
};

extern TranspositionTable TT;

// Size は 2のべき乗であること。
template <typename T, size_t Size>
struct HashTable 
{
	HashTable() : entries_(Size, T()) {}
	T* operator [] (const Key k) { return &entries_[static_cast<size_t>(k) & (Size - 1)]; }
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

	void save(const Key k, const Score s, const int remain_help) 
	{
		// scoreの29～31ビット目が1になることは想定していない．またスコアがマイナスになることも想定していない．
		assert(!(s & 0xe0000000) && s >= 0);
		word = static_cast<Key>(k >> 32) 
			 | static_cast<Key>(static_cast<int64_t>(s) << 32)
			 | static_cast<Key>(remain_help) << 61;
	}

	Key word;
};

struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};

// フィールドの連鎖情報
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