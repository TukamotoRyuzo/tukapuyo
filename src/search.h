#pragma once
#include <iostream>
#include <tuple>
#include "debug.h"

struct SignalsType 
{
	bool stop_on_ponderhit, first_root_move, stop, failed_low_at_root;
};

struct LimitsType 
{
	LimitsType() { std::memset(this, 0, sizeof(LimitsType)); }
	bool UseTimeManagement() const { return !(mate | movetime | depth | nodes | infinite); }

	int time, depth, nodes, movetime, mate, infinite, ponder;
};


class RootMove
{
public: 
	RootMove(){};

	explicit RootMove(const Move m) : score_(-SCORE_INFINITE), prev_score_(-SCORE_INFINITE)
	{
		pv_.push_back(m);
		pv_.push_back(Move::moveNone());
	}
	explicit RootMove(const std::tuple<Move, Score> m) : score_(std::get<1>(m)), prev_score_(-SCORE_INFINITE)
	{
		pv_.push_back(std::get<0>(m));
		pv_.push_back(Move::moveNone());
	}
	void extractPvFromTT(LightField self, LightField enemy, int remain_time);
	void insertPvInTT(LightField self, LightField enemy, int remain_time);
	bool operator < (const RootMove& m) const { return score_ < m.score_; }
	bool operator == (const Move& m) const { return pv_[0] == m; }

	int16_t score_;
	int16_t prev_score_;
	std::vector<Move> pv_;
	std::vector<Flag> player_;
};

struct Searcher
{
	static Searcher* thisptr;

	// 探索の強制停止信号などのフラグの集合体。
	static volatile SignalsType signals;

	// 思考時間等の設定項目
	static LimitsType limits;

	// 探索開始局面の指し手
	static std::vector<Move> search_moves;

	// 時間を計るクラス
	static Time search_timer;

	static std::vector<RootMove> root_moves;

	// 探索開始局面
	static LightField root_self;
	static LightField root_enemy;
};

// 汎用的な insertion sort. 要素数が少ない時、高速にソートできる。
// 降順(大きいものが先頭付近に集まる)
// *(first - 1) に 番兵(sentinel) として MAX 値が入っていると仮定して高速化してある。
// T には ポインタかイテレータを使用出来る。
template <typename T> inline void insertionSort(T first, T last)
{
	if (first != last)
	{
		for (T curr = first + 1; curr != last; ++curr)
		{
			if (*(curr - 1) < *curr)
			{
				const auto tmp = std::move(*curr);

				do {
					*curr = *(curr - 1);
					--curr;
				} while ((curr != first) && *(curr - 1) < tmp);
				*curr = std::move(tmp);
			}
		}
	}
}