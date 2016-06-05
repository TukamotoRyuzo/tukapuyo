#include "AI.h"
#include "tt.h"
#include "search.h"
#include "debug.h"
#include "move.h"
#include <vector>
#include <cassert>
#include <cmath>
#include "chain.h"

//#define TIME_MANAGE

namespace
{	
	int64_t time_start;
}

// レベルを設定する．
void PolytecAI::setLevel(int level)
{
	assert(level >= 1 && level <= 6);

	if(level == 1)
	{
		depth_max_recieve_ = 1;
		easy_ = 2;
	}
	else if(level == 2)
	{
		depth_max_recieve_ = 1;
		easy_ = 1;
	}
	else if(level == 3)
	{
		depth_max_recieve_ = 4;
		easy_ = 1;
	}
	else if(level == 4)
	{
		depth_max_recieve_ = 4;
		easy_ = 0;
	}
	else if(level == 5)
	{
		depth_max_recieve_ = 2;
		easy_ = 0;
	}
	else
	{
		depth_max_recieve_ = 4;
		easy_ = 0;
	}
	level_ = level;
}

int AI::thinkWrapperEX(Field self, Field enemy)
{

	// 静かな局面になるまで局面を進める
	int my_remain_time = enemy.generateStaticState(self);

	// enemyが死んでいる局面の場合は-1が返ってくる
	if (my_remain_time == -1)
	{
		if (enemy.isDeath())
		{
			operate_.clear();

			// その時は探索は行わず、ずっと回転し続けることにする（煽りではない）
			for (int i = 0; i < 1000; i++)
			{
				if (i % 7 == 0)
					operate_.push(R_ROTATE);
				else
					operate_.push(0);
			}
			return 0;
		}
	}

	// 進めたことによりエラーが発生しないかどうか。
	assert(self.examine());
	assert(enemy.examine());

	// 探索用に小さなサイズのフィールドにする
	LightField s(self);
	LightField e(enemy);

	assert(s.chainMax() == 0 && e.chainMax() == 0);
	assert(s.examine());
	assert(e.examine());

	ChainsList* c1 = new ChainsList(100);
	ChainsList* c2 = new ChainsList(100);

	s.setChains(c1);
	e.setChains(c2);

	// 世代を進める
	TT.newSearch();                             

    Score alpha = SCORE_LOSE - Score(1);	// α:下限値
	Score beta  = SCORE_WIN  + Score(1);	// β:上限値
	Score delta = alpha;
	Score score = SCORE_ZERO;
	continue_self_num_ = 0;
	
	Move best_move;
	time_start = system_time_to_msec();

	Move mlist[22];
	int mcount = s.generateMoves(mlist);

	// 死んでいる局面ではないはず
	assert(mcount > 0);

	root_moves.clear();

	for (int i = 0; i < mcount; ++i)
	{
		root_moves.push_back(RootMove(mlist[i]));
		root_moves[i].player_.push_back(self.player());
	}
	
	// aspiration searchでfail low / fail highを起こした回数．これが少なくてしかも小さいdeltaであればよい
	int aspiration_miss = 0;

	// 反復深化
	for (depth_max_ = 1; depth_max_ <= depth_max_recieve_; depth_max_++)
	{
		try
		{
			// 前回のiterationでの指し手の点数をすべてコピー
			for (int i = 0; i < root_moves.size(); ++i)
				root_moves[i].prev_score_ = root_moves[i].score_;

			// aspiration search
			// alpha betaをある程度絞ることで、探索効率を上げる。
			if (3 <= depth_max_ && abs(root_moves[0].prev_score_) < SCORE_WIN)
			{
				delta = static_cast<Score>(5);
				alpha = static_cast<Score>(root_moves[0].prev_score_) - delta;
				beta = static_cast<Score>(root_moves[0].prev_score_) + delta;
			}
			else
			{
				alpha = SCORE_LOSE - Score(1);
				beta = SCORE_WIN + Score(1);
			}

			// aspiration search のwindow幅をはじめは小さい値にして探索し、
			// fail high/lowになったなら、今度はwindow幅を広げて再探索を行う。
			while (true)
			{
				// 探索開始
				score = search<ROOT>(alpha, beta, s, e, 0, my_remain_time);

				// 先頭が最善手になるようにソート
				insertionSort(root_moves.begin(), root_moves.end());

				// fail high / lowが起きなかった場合はループを抜ける。
				if (alpha < score && score < beta)
					break;

				// fail low/highが起きた場合、aspiration窓を増加させ再探索し、
				// さもなくばループを抜ける。
				if (abs(score) >= Score(10000))
				{
					// 勝ちか負けだと判定したら、最大の幅で探索を試してみる。
					alpha = SCORE_LOSE - Score(1);
					beta = SCORE_WIN + Score(1);
				}
				else if (beta <= score)
				{
					aspiration_miss++;
					// fail highならば、これ以上の値が期待できるわけでありponder hitすればこれをbest moveとして
					// 返しても問題がないはずで、上のような処理はなく、単に上側をdeltaだけ広げる。
					beta += delta;
					delta += delta / 2;
				}
				else
				{
					aspiration_miss++;
					// fail lowしているので、下側をさらにdeltaだけ広げる
					alpha -= delta;
					delta += delta / 2;
				}
			}

			// fail low / highをなるべく起こさないように調整する．
			MyOutputDebugString("asp_miss = %d ", aspiration_miss);
			aspiration_miss = 0;
			best_move = best_[0];

			MyOutputDebugString("score = %d, depth = %d, best = ", score, depth_max_);

			int s_field_ply = 0;
			int e_field_ply = 0;

			// pv表示
			for (int size = 0; size < root_moves[0].pv_.size() - 1; size++)
			{
				LightField* now_player = root_moves[0].player_[size] == self.player() ? &self : &enemy;

				int now_field_ply;

				if (now_player == &self)
					now_field_ply = s_field_ply++;
				else
					now_field_ply = e_field_ply++;
				
				MyOutputDebugString("%s%s,",
					root_moves[0].player_[size] == PLAYER1 ? "P1:" : "P2:",
					root_moves[0].pv_[size].toString(*now_player, now_field_ply).c_str());
			}
			MyOutputDebugString("\n");			
		}
		catch(int)
		{
			MyOutputDebugString("terminated depth = %d", depth_max_);
			break;
		}
	}
	MyOutputDebugString("\n");

	// 探索した結果得られた手を実際に配置できるかどうか。
	assert(self.isEmpty(best_move.psq()) && self.isEmpty(best_move.csq()));

	// best_moveを操作に変換
	operate_.generate(best_move, self);

	delete c1;
	delete c2;

	return score;
}

// ネガα探索
template <NodeType NT>
Score AI::search(Score alpha, Score beta, LightField& self, LightField& enemy, int depth, int my_remain_time)
{
	node_searched++;
	const bool rootnode = NT == ROOT;

	assert(alpha >= SCORE_LOSE - 1 && alpha < beta && beta <= SCORE_WIN + 1);

#ifdef TIME_MANAGE
	if (depth == 1)
	{
		int64_t time_end = system_time_to_msec();
		if (time_end - time_start >= 5000)
		{
			throw 0;
		}
	}
#endif
	// 残り深さ
	int remain_depth = depth_max_ - depth;

	// 局面表を見る
	Move tt_move, best_move;
	Score tt_score;
	Key key = self.key() ^ enemy.key();
	const TTEntry *tte = TT.probe(self, enemy);

	// 局面表の指し手
	if (rootnode)
	{
		if (depth_max_ == 1)
		{
			tt_move = tte ? tte->move() : Move::moveNone();
		}
		else
		{
			tt_move = root_moves[0].pv_[0];
		}
	}
	else
	{
		tt_move = tte ? tte->move() : Move::moveNone();
	}

	// 置換表上のスコア
	tt_score = tte ? tte->score() : SCORE_NONE;

	// 置換表のスコアが信用に足るならそのまま返す。
	// 条件)
	// root nodeではない
	// 置換表にエントリーがあった
	// 置換表のエントリーのdepthが今回の残り探索depth以上である
	// 置換表のエントリーのスコアがSCORE_NONEではない。(置換表のアクセス競合のときにのみこの値になりうる)
	// 置換表のスコアのboundがBOUND_EXACTなら信用に足るので置換表の指し手をそのまま返す
	// non PV nodeであれば置換表の値がbetaを超えているならBOUND_LOWER(真の値はこれより上なのでこのときbeta cutできる)であるか、
	// betaを超えていないのであれば、BOUND_UPPER(真の値はこれより下なのでこのときbeta cutが起きないことが確定)である。

	if (!rootnode 
		&& tte != nullptr
		&& tte->remainDepth() >= remain_depth
		&& (remain_depth == 0
		|| (tte->bound() == BOUND_EXACT
		|| (tte->bound() == BOUND_LOWER && tt_score >= beta))))
	{		
		// この置換表の指し手が役に立ったので、置換表のエントリーの世代を現在の世代にする。
		TT.refresh(tte);

		best_[depth] = tte->move();

		assert(tte->move().isNone() || tte->move().isLegal(self));

		// 局面表のスコアは信用できるのでそのまま返す
		return tt_score;
	}

	// 最大探索深さまでいったら、その局面の点数を返す
	if (depth >= depth_max_)
	{
		Score s = evaluateEX(self, enemy, depth, my_remain_time);

		// せっかく評価関数を呼び出したので局面表に登録しておく。
		TT.store(key, s, 0, Move::moveNone(), BOUND_NONE, self.player(), my_remain_time, self.ojama() - enemy.ojama());
		return s;
	}

	Move move[23];
	int move_max = 0;
	Move *pmove = move;

	if (!tt_move.isNone())
	{
		if (tt_move.isLegalAbout(self))
		{
			// 置換表にある手を手のバッファの先頭に入れる。
			*pmove++ = tt_move;
			move_max++;
		}
	}

	if ((move_max += self.generateMoves(pmove)) == 0)
	{
		// 置く場所なし == 負け
		return SCORE_LOSE;
	}
		
	Score score, max = SCORE_LOSE - Score(1);
	
	// お邪魔ぷよが降る場合はenemyのojamaを減らしているので、
	// このmoveを調べ終わった後元に戻さなければならない
	int enemy_ojama_prev = enemy.ojama();
	Flag enemy_flag = enemy.flag();
	self.nextPlus();

	// TODO:ムーブオーダリングしたい

	// 手がある間、すべての手を試す
	for (int i = 0; i < move_max; i++)
	{
		// 局面のコピーを用意
		LightField self2(self);

		// 着手。連鎖が起きる場合は起きた後の形になるまで進めてしまう。
		int put_time = self2.doMoveEX(move[i], my_remain_time, enemy);

		// 自分の残り時間が0以下なら、相手の手盤になると考える
		if (my_remain_time - put_time <= 0)
		{
			continue_self_num_ = 0;

			// 相手の探索をする。残り時間の超過分は、相手の残り時間になる。
			score = -search<PV>(-beta, -alpha, enemy, self2, depth + 1, -(my_remain_time - put_time));
		}
		else
		{
			continue_self_num_++;

			// 連続3回以上自分の探索をした場合はこれ以上深く探索しないために残り深さを減らす。(重すぎるので)
			if (continue_self_num_ >= 3)
			{
				score = search<PV>(alpha, beta, self2, enemy, depth_max_, my_remain_time - put_time);
			}
			else
			{
				// 残り時間が残っている間は、連続して自分の探索をする
				score = search<PV>(alpha, beta, self2, enemy, depth + 1, my_remain_time - put_time);
			}

			continue_self_num_--;
		}
		
		enemy.setOjama(enemy_ojama_prev);
		enemy.resetFlag(enemy_flag);

		if (rootnode)
		{
			// Root nodeでの指し手のなかから、いま探索したばかりの指し手に関してそれがどこにあるかをfind()で探し、
			// この指し手に付随するPV(最善応手列)を置換表から取り出して更新しておく。
			// このfind()が失敗することは想定していない。
			RootMove& rm = *std::find(root_moves.begin(), root_moves.end(), move[i]);

			if (score > alpha)
			{
				rm.score_ = score;

				// 局面表をprobeするときにツモ番号を元に戻す必要があるので。
				self.nextMinus();
				rm.extractPvFromTT(self, enemy, my_remain_time);
				self.nextPlus();
			}
			else
			{
				// tt_moveがセットされているなら一番最初に調べられたはず。
				// ここにくるということはtt_moveが2回調べられているということ。
				if (move[i] != tt_move)
				{
					rm.score_ = SCORE_LOSE;
				}
			}
		}

		if (score > max)
		{
			max = score;

			if (max > alpha)
			{				
				alpha = max;

				if (alpha >= beta)
				{
					// βカットされたalphaという値は正確な値ではないが、そのノードの評価値は最低でもalpha以上だということがわかる。
					// なので、alphaはそのノードの下限値である。だからBOUND_LOWER
					TT.store(key, alpha, remain_depth, move[i], BOUND_LOWER, self.player(), my_remain_time, self.ojama() - enemy.ojama());
					self.nextMinus();
					return max;
				}				
			}
			// 最も評価の高かった手を採用する。
			best_[depth] = move[i];
		}
	}

	self.nextMinus();

	// 最大でも評価値はmaxまでしか行かない
	TT.store(key, max, remain_depth, best_[depth], BOUND_EXACT, self.player(), my_remain_time, self.ojama() - enemy.ojama());
	return max;

}

Score AI::evaluateEX(LightField &self, LightField &enemy, int depth, int my_remain_time)
{
	eval_called++;

	Score score = self.evaluate(enemy, my_remain_time);

	if (score >= SCORE_WIN)
		score = SCORE_WIN;
	else if (score <= SCORE_LOSE)
		score = SCORE_LOSE;

	return score;
}

// pv構築する。置換表からpvを得るので置換表をprobeしている。表示情報を得る以外のメリットはないかもしれない。
void RootMove::extractPvFromTT(LightField self, LightField enemy, int remain_time)
{
	int ply = 0;
	Move m = pv_[0];
	Flag player = self.player();
	const Flag selfplayer = player;
	const Flag enemyplayer = enemy.player();
	const TTEntry* tte;
	pv_.clear();
	pv_.push_back(m);
	player_.clear();
	player_.push_back(player);
	assert(self.examine());
	assert(enemy.examine());
	assert(m.isLegalAbout(self));
	assert(m.isLegal(self));
	self.nextPlus();
	remain_time -= self.doMoveEX(m, remain_time, enemy);

	if (remain_time > 0)
		tte = TT.probe(self, enemy);
	else
		tte = TT.probe(enemy, self);

	assert(tte != nullptr);

	m = tte->move();
	player = tte->player();

	if (m.isNone())
	{
		pv_.push_back(Move::moveNone());
		return;
	}

	assert(m.isLegal(player == selfplayer ? self : enemy));
	assert(m.isLegalAbout(player == selfplayer ? self : enemy));

	do{
		pv_.push_back(m);
		player_.push_back(player);

		if (player == selfplayer)
		{
			self.nextPlus();
			remain_time -= self.doMoveEX(m, remain_time, enemy);

			if (remain_time > 0)
				tte = TT.probe(self, enemy);
			else 
				tte = TT.probe(enemy, self);
		}
		else
		{
			enemy.nextPlus();
			remain_time += enemy.doMoveEX(m, -remain_time, self);

			if (remain_time < 0)
				tte = TT.probe(enemy, self);
			else
				tte = TT.probe(self, enemy);				
		}

		assert(self.examine());
		assert(enemy.examine());

		if (tte != nullptr)
		{
			m = tte->move();
			player = tte->player();
			if (!m.isNone())
			{
				Key key = self.key() ^ enemy.key();
				assert(m.isLegalAbout(player == selfplayer ? self : enemy));
				assert(m.isLegal(player == selfplayer ? self : enemy));
			}
		}
	} while (tte != nullptr 
		&& !m.isNone() // 評価値だけ登録されている場合もある。
		&& m.isLegal(player == selfplayer ? self : enemy)
		&& m.isLegalAbout(player == selfplayer ? self : enemy));

	pv_.push_back(Move::moveNone());
}