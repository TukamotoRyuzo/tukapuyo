#include "search.h"
#include "thread.h"
#include "tt.h"

SignalsType Signals;
LimitsType Limits;
using namespace Search;

const int MAX_PLY = 128;


namespace
{
	typedef std::vector<int> Row;

	const Row HalfDensity[] =
	{
		{ 0, 1 },
		{ 1, 0 },
		{ 0, 0, 1, 1 },
		{ 0, 1, 1, 0 },
		{ 1, 1, 0, 0 },
		{ 1, 0, 0, 1 },
		{ 0, 0, 0, 1, 1, 1 },
		{ 0, 0, 1, 1, 1, 0 },
		{ 0, 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0, 0 },
		{ 1, 1, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 1, 1 },
		{ 0, 0, 0, 0, 1, 1, 1, 1 },
		{ 0, 0, 0, 1, 1, 1, 1, 0 },
		{ 0, 0, 1, 1, 1, 1, 0 ,0 },
		{ 0, 1, 1, 1, 1, 0, 0 ,0 },
		{ 1, 1, 1, 1, 0, 0, 0 ,0 },
		{ 1, 1, 1, 0, 0, 0, 0 ,1 },
		{ 1, 1, 0, 0, 0, 0, 1 ,1 },
		{ 1, 0, 0, 0, 0, 1, 1 ,1 },
	};
	const size_t HalfDensitySize = std::extent<decltype(HalfDensity)>::value;
	template <NodeType NT> 
	Score search(LightField &self, LightField& enemy, Stack* ss, Score alpha, Score beta, Depth depth);
} // namespace

void Search::init();
void Search::clear();

void MainThread::search()
{

}

void Thread::search()
{
	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	Score best_score, alpha, beta, delta;
	MainThread* main_thread = (this == Threads.main() ? Threads.main() : nullptr);

	std::memset(ss - 5, 0, 8 * sizeof(Stack));

	best_score = delta = alpha = -SCORE_INFINITE;
	beta = SCORE_INFINITE;
	completed_depth = DEPTH_ZERO;

	if (main_thread)
	{
		TT.newSearch();
	}

	while (++root_depth < DEPTH_MAX && !Signals.stop && (!Limits.depth || root_depth <= Limits.depth))
	{
		// ヘルパースレッドのための新しい深さを設定する。半密度行列というものを使う
		if (!main_thread)
		{
			const Row& row = HalfDensity[(idx - 1) % HalfDensitySize];

			if (row[root_depth % row.size()])
				continue;

			// PV変動率を計算する
			if (main_thread)
				main_thread->best_move_changes *= 0.505, main_thread->failed_low = false;

			for (RootMove& rm : root_moves)
				rm.previous_score = rm.score;

			if (root_depth >= 5 * ONE_PLY)
			{
				delta = Score(5);
				alpha = std::max(root_moves[0].previous_score - delta, -SCORE_INFINITE);
				beta = std::min(root_moves[0].previous_score + delta, SCORE_INFINITE);
			}

			while (true)
			{
				best_score = ::search<PV>(root_field_self, root_field_enemy, ss, alpha, beta, root_depth);

				std::stable_sort(root_moves.begin(), root_moves.end());

				// pvを置換表に書いておく
				//root_moves[0].insertPvInTT(root_board_self, root_field_enemy, );

				// 勝ちを見つけたら速攻着手
				if (best_score >= SCORE_MATE || best_score >= SCORE_KNOWN_WIN && root_moves.size() == 1)
				{
					root_moves[0].score = SCORE_INFINITE;
					return;
				}

				if (Signals.stop)
					break;

				// 探索をする前にUIに出力する
				//if (main_thread
				//	&& (best_score <= alpha || best_score >= beta)
				//	&& Time.elapsed() > 3000)
				//	SYNC_COUT << USI::pv(root_board, root_depth, alpha, beta) << SYNC_ENDL;

				// fail high/lowを起こしているならwindow幅を広げる
				if (best_score <= alpha)
				{
					beta = (alpha + beta) / 2;
					alpha = std::max(best_score - delta, -SCORE_INFINITE);

					if (main_thread)
					{
						// 安定していないので
						main_thread->failed_low = true;
						Signals.stop_on_ponderhit = false;
					}
				}
				else if (best_score >= beta)
				{
					alpha = (alpha + beta) / 2;
					beta = std::min(best_score + delta, SCORE_INFINITE);
				}
				else
					break;

				delta += delta / 4 + 5;

				assert(alpha >= -SCORE_INFINITE && beta <= SCORE_INFINITE);
			}
		}
	}

}

namespace
{
	template <NodeType NT>
	Score search(LightField &self, LightField& enemy, Stack* ss, Score alpha, Score beta, Depth depth)
	{
		return SCORE_ZERO;
	}
}