#pragma once
#include <iostream>
#include <tuple>
#include "debug.h"

struct SignalsType { bool stop_on_ponderhit, stop; };

struct LimitsType 
{
	LimitsType() { std::memset(this, 0, sizeof(LimitsType)); }
	bool UseTimeManagement() const { return !(depth | nodes); }
	int time, depth, nodes, ponder;
};

class RootMove
{
public: 
	RootMove(){};

	/*explicit RootMove(const Move m) : score_(-SCORE_INFINITE), prev_score_(-SCORE_INFINITE)
	{
		pv_.push_back(m);
		pv_.push_back(Move::moveNone());
	}
	explicit RootMove(const std::tuple<Move, Score> m) : score_(std::get<1>(m)), prev_score_(-SCORE_INFINITE)
	{
		pv_.push_back(std::get<0>(m));
		pv_.push_back(Move::moveNone());
	}*/
	explicit RootMove(Move m) : pv_(1, m) {}

	void extractPvFromTT(LightField self, LightField enemy, int remain_time);
	void insertPvInTT(LightField self, LightField enemy, int remain_time);
	bool operator < (const RootMove& m) const { return score_ < m.score_; }
	bool operator == (const Move& m) const { return pv_[0] == m; }

	Score score_ = -SCORE_INFINITE;
	Score prev_score_ = -SCORE_INFINITE;

	std::vector<Move> pv_;
	std::vector<Flag> player_;
};

// �ėp�I�� insertion sort. �v�f�������Ȃ����A�����Ƀ\�[�g�ł���B
// �~��(�傫�����̂��擪�t�߂ɏW�܂�)
// *(first - 1) �� �ԕ�(sentinel) �Ƃ��� MAX �l�������Ă���Ɖ��肵�č��������Ă���B
// T �ɂ� �|�C���^���C�e���[�^���g�p�o����B
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