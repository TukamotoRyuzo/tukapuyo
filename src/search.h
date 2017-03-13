#pragma once

#include "debug.h"
#include "move.h"
#include "const.h"

enum NodeType { ROOT, PV, NO_PV };
struct SignalsType { bool stop_on_ponderhit, stop; };

// 操作中にも思考したかったら必要になるかも
struct LimitsType
{
	LimitsType() { std::memset(this, 0, sizeof(LimitsType)); }
	bool UseTimeManagement() const { return !(depth | nodes); }
	int time, depth, nodes, ponder;
};

enum Depth : int16_t
{
	ONE_PLY = 1,

	DEPTH_ZERO = 0,
	DEPTH_QS_CHECKS = 0,
	DEPTH_QS_NO_CHECKS = -1,
	DEPTH_QS_RECAPTURES = -5,

	DEPTH_NONE = -6,
	DEPTH_MAX = 128,
};

ENABLE_OPERATORS_ON(Depth);

namespace Search
{
	struct Stack
	{
		Move* pv;
		int ply; // ルートからの深さ
		Move current_move;
		Move excluded_move;
		Move killers[2];
		Score static_eval; // 現局面で評価関数を呼び出した時のスコア
		bool skip_early_pruning;
		int move_count;
	};

	struct RootMove
	{
		RootMove(){};

		explicit RootMove(Move m) : pv(1, m) {}

		void insertPvInTT(LightField self, LightField enemy, int remain_time);
		bool operator < (const RootMove& m) const { return score < m.score; }
		bool operator == (const Move& m) const { return pv[0] == m; }

		Score score = -SCORE_INFINITE;
		Score previous_score = -SCORE_INFINITE;

		std::vector<Move> pv;
		std::vector<Flag> player;
	};
} // namespace Search

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