#include "AI.h"
#include "tt.h"
#include "search.h"
#include "debug.h"
#include "move.h"
#include <vector>
#include <cassert>
#include <cmath>
#include "chain.h"

// ���x����ݒ肷��D
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
	// �Â��ȋǖʂɂȂ�܂ŋǖʂ�i�߂�
	int my_remain_time = enemy.generateStaticState(self);

	// enemy������ł���ǖʂ̏ꍇ��-1���Ԃ��Ă���
	if (my_remain_time == -1)
	{
		if (enemy.isDeath())
		{
			operate_.clear();

			// ���̎��͒T���͍s�킸�A�����Ɖ�]�������邱�Ƃɂ���i����ł͂Ȃ��j
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

	// �i�߂����Ƃɂ��G���[���������Ȃ����ǂ����B
	assert(self.examine());
	assert(enemy.examine());

	// �T���p�ɏ����ȃT�C�Y�̃t�B�[���h�ɂ���
	LightField s(self);
	LightField e(enemy);

	assert(s.chainMax() == 0 && e.chainMax() == 0);
	assert(s.examine());
	assert(e.examine());

	ChainsList* c1 = new ChainsList(100);
	ChainsList* c2 = new ChainsList(100);

	s.setChains(c1);
	e.setChains(c2);

	// �����i�߂�
	TT.newSearch();                             

    Score alpha = -SCORE_INFINITE;	// ��:�����l
	Score beta  = SCORE_INFINITE;	// ��:����l
	Score delta = alpha;
	Score score = SCORE_ZERO;
	continue_self_num_ = 0;
	
	Move best_move;

	Move mlist[22];
	int mcount = s.generateMoves(mlist);

	// ����ł���ǖʂł͂Ȃ��͂�
	assert(mcount > 0);

	root_moves.clear();

	for (int i = 0; i < mcount; ++i)
	{
		root_moves.push_back(Search:: RootMove(mlist[i]));
		root_moves[i].player.push_back(self.player());
	}
	
	// aspiration search��fail low / fail high���N�������񐔁D���ꂪ���Ȃ��Ă�����������delta�ł���΂悢
	int aspiration_miss = 0;

	// �����[��
	for (depth_max_ = 1; depth_max_ <= depth_max_recieve_; depth_max_++)
	{
		try
		{
			// �O���iteration�ł̎w����̓_�������ׂăR�s�[
			for (int i = 0; i < root_moves.size(); ++i)
				root_moves[i].previous_score = root_moves[i].score;

			// aspiration search
			// alpha beta��������x�i�邱�ƂŁA�T���������グ��B
			if (3 <= depth_max_ && abs(root_moves[0].previous_score) < SCORE_INFINITE)
			{
				delta = static_cast<Score>(5);
				alpha = static_cast<Score>(root_moves[0].previous_score) - delta;
				beta = static_cast<Score>(root_moves[0].previous_score) + delta;
			}
			else
			{
				alpha = -SCORE_INFINITE;
				beta = SCORE_INFINITE;
			}

			// aspiration search ��window�����͂��߂͏������l�ɂ��ĒT�����A
			// fail high/low�ɂȂ����Ȃ�A���x��window�����L���čĒT�����s���B
			while (true)
			{
				// �T���J�n
				score = search<ROOT>(alpha, beta, s, e, 0, my_remain_time);

				// �擪���őP��ɂȂ�悤�Ƀ\�[�g
				insertionSort(root_moves.begin(), root_moves.end());

				// fail high / low���N���Ȃ������ꍇ�̓��[�v�𔲂���B
				if (alpha < score && score < beta)
					break;

				// fail low/high���N�����ꍇ�Aaspiration���𑝉������ĒT�����A
				// �����Ȃ��΃��[�v�𔲂���B
				if (abs(score) >= Score(10000))
				{
					// �������������Ɣ��肵����A�ő�̕��ŒT���������Ă݂�B
					alpha = -SCORE_INFINITE;
					beta = SCORE_INFINITE;
				}
				else if (beta <= score)
				{
					aspiration_miss++;
					// fail high�Ȃ�΁A����ȏ�̒l�����҂ł���킯�ł���ponder hit����΂����best move�Ƃ���
					// �Ԃ��Ă���肪�Ȃ��͂��ŁA��̂悤�ȏ����͂Ȃ��A�P�ɏ㑤��delta�����L����B
					beta += delta;
					delta += delta / 2;
				}
				else
				{
					aspiration_miss++;
					// fail low���Ă���̂ŁA�����������delta�����L����
					alpha -= delta;
					delta += delta / 2;
				}
			}

			// fail low / high���Ȃ�ׂ��N�����Ȃ��悤�ɒ�������D
			MyOutputDebugString("asp_miss = %d ", aspiration_miss);
			aspiration_miss = 0;
			best_move = best_[0];

			MyOutputDebugString("score = %d, depth = %d, best = ", score, depth_max_);

			int s_field_ply = 0;
			int e_field_ply = 0;

			// pv�\��
			for (int size = 0; size < root_moves[0].pv.size() - 1; size++)
			{
				LightField* now_player = root_moves[0].player[size] == self.player() ? &self : &enemy;

				int now_field_ply;

				if (now_player == &self)
					now_field_ply = s_field_ply++;
				else
					now_field_ply = e_field_ply++;
				
				MyOutputDebugString("%s%s,",
					root_moves[0].player[size] == PLAYER1 ? "P1:" : "P2:",
					root_moves[0].pv[size].toString(*now_player, now_field_ply).c_str());
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

	// �T���������ʓ���ꂽ������ۂɔz�u�ł��邩�ǂ����B
	assert(self.isEmpty(best_move.psq()) && self.isEmpty(best_move.csq()));

	// best_move�𑀍�ɕϊ�
	operate_.generate(best_move, self);

	delete c1;
	delete c2;

	return score;
}

// �l�K���T��
template <NodeType NT>
Score AI::search(Score alpha, Score beta, LightField& self, LightField& enemy, int depth, int my_remain_time)
{
	node_searched++;
	const bool rootnode = NT == ROOT;

	assert(alpha >= -SCORE_INFINITE && alpha < beta && beta <= SCORE_INFINITE);

	// �c��[��
	int remain_depth = depth_max_ - depth;

	// �ǖʕ\������
	Move tt_move, best_move;
	Score tt_score;
	TTEntry* tte;
	const Key key = self.key() ^ enemy.key();
	const bool tt_hit = TT.probe<false>(&self, &enemy, tte);

	// �ǖʕ\�̎w����
	if (rootnode)
	{
		if (depth_max_ == 1)
			tt_move = tt_hit ? tte->move() : Move::moveNone();
		else
			tt_move = root_moves[0].pv[0];
	}
	else
		tt_move = tt_hit ? tte->move() : Move::moveNone();

	// �u���\��̃X�R�A
	tt_score = tt_hit ? tte->score() : SCORE_NONE;

	// �u���\�̃X�R�A���M�p�ɑ���Ȃ炻�̂܂ܕԂ��B
	// ����)
	// root node�ł͂Ȃ�
	// �u���\�ɃG���g���[��������
	// �u���\�̃G���g���[��depth������̎c��T��depth�ȏ�ł���
	// �u���\�̃G���g���[�̃X�R�A��SCORE_NONE�ł͂Ȃ��B(�u���\�̃A�N�Z�X�����̂Ƃ��ɂ݂̂��̒l�ɂȂ肤��)
	// �u���\�̃X�R�A��bound��BOUND_EXACT�Ȃ�M�p�ɑ���̂Œu���\�̎w��������̂܂ܕԂ�
	// non PV node�ł���Βu���\�̒l��beta�𒴂��Ă���Ȃ�BOUND_LOWER(�^�̒l�͂������Ȃ̂ł��̂Ƃ�beta cut�ł���)�ł��邩�A
	// beta�𒴂��Ă��Ȃ��̂ł���΁ABOUND_UPPER(�^�̒l�͂����艺�Ȃ̂ł��̂Ƃ�beta cut���N���Ȃ����Ƃ��m��)�ł���B

	if (!rootnode 
		&& tt_hit
		&& tte->depth() >= remain_depth
		&& (remain_depth == 0
		|| (tte->bound() == BOUND_EXACT
		|| (tte->bound() == BOUND_LOWER && tt_score >= beta))))
	{		
		best_[depth] = tte->move();

		assert(tte->move().isNone() || tte->move().isLegal(self));

		// �ǖʕ\�̃X�R�A�͐M�p�ł���̂ł��̂܂ܕԂ�
		return tt_score;
	}

	// �ő�T���[���܂ł�������A���̋ǖʂ̓_����Ԃ�
	if (depth >= depth_max_)
	{
		Score s = evaluateEX(self, enemy, depth, my_remain_time);

		// ���������]���֐����Ăяo�����̂ŋǖʕ\�ɓo�^���Ă����B
		tte->save(key, 0, s, 0, BOUND_NONE, TT.generation(), self.player(), my_remain_time, self.ojama() - enemy.ojama());
		return s;
	}

	Move move[23];
	int move_max = 0;
	Move *pmove = move;

	if (!tt_move.isNone())
	{
		if (tt_move.isLegalAbout(self))
		{
			// �u���\�ɂ�������̃o�b�t�@�̐擪�ɓ����B
			*pmove++ = tt_move;
			move_max++;
		}
	}

	if ((move_max += self.generateMoves(pmove)) == 0)
	{
		// �u���ꏊ�Ȃ� == ����
		return SCORE_MATED;
	}
		
	Score score, max = -SCORE_INFINITE;
	
	// ���ז��Ղ悪�~��ꍇ��enemy��ojama�����炵�Ă���̂ŁA
	// ����move�𒲂׏I������㌳�ɖ߂��Ȃ���΂Ȃ�Ȃ�
	int enemy_ojama_prev = enemy.ojama();
	Flag enemy_flag = enemy.flag();
	self.nextPlus();

	// TODO:���[�u�I�[�_�����O������

	// �肪����ԁA���ׂĂ̎������
	for (int i = 0; i < move_max; i++)
	{
		// �ǖʂ̃R�s�[��p��
		LightField self2(self);

		// ����B�A�����N����ꍇ�͋N������̌`�ɂȂ�܂Ői�߂Ă��܂��B
		int put_time = self2.doMoveEX(move[i], my_remain_time, enemy);

		// �����̎c�莞�Ԃ�0�ȉ��Ȃ�A����̎�ՂɂȂ�ƍl����
		if (my_remain_time - put_time <= 0)
		{
			continue_self_num_ = 0;

			// ����̒T��������B�c�莞�Ԃ̒��ߕ��́A����̎c�莞�ԂɂȂ�B
			score = -search<PV>(-beta, -alpha, enemy, self2, depth + 1, -(my_remain_time - put_time));
		}
		else
		{
			continue_self_num_++;

			// �A��3��ȏ㎩���̒T���������ꍇ�͂���ȏ�[���T�����Ȃ����߂Ɏc��[�������炷�B(�d������̂�)
			if (continue_self_num_ >= 3)
			{
				score = search<PV>(alpha, beta, self2, enemy, depth_max_, my_remain_time - put_time);
			}
			else
			{
				// �c�莞�Ԃ��c���Ă���Ԃ́A�A�����Ď����̒T��������
				score = search<PV>(alpha, beta, self2, enemy, depth + 1, my_remain_time - put_time);
			}

			continue_self_num_--;
		}
		
		enemy.setOjama(enemy_ojama_prev);
		enemy.resetFlag(enemy_flag);

		if (rootnode)
		{
			// Root node�ł̎w����̂Ȃ�����A���ܒT�������΂���̎w����Ɋւ��Ă��ꂪ�ǂ��ɂ��邩��find()�ŒT���A
			// ���̎w����ɕt������PV(�őP�����)��u���\������o���čX�V���Ă����B
			// ����find()�����s���邱�Ƃ͑z�肵�Ă��Ȃ��B
			Search:: RootMove& rm = *std::find(root_moves.begin(), root_moves.end(), move[i]);

			if (score > alpha)
			{
				rm.score = score;

				// �ǖʕ\��probe����Ƃ��Ƀc���ԍ������ɖ߂��K�v������̂ŁB
				/*self.nextMinus();
				rm.extractPvFromTT(self, enemy, my_remain_time);
				self.nextPlus();*/
			}
			else
			{
				// tt_move���Z�b�g����Ă���Ȃ��ԍŏ��ɒ��ׂ�ꂽ�͂��B
				// �����ɂ���Ƃ������Ƃ�tt_move��2�񒲂ׂ��Ă���Ƃ������ƁB
				if (move[i] != tt_move)
				{
					rm.score = -SCORE_INFINITE;
				}
			}
		}

		if (score > max)
		{
			max = score;
			best_[depth] = move[i];	

			if (max > alpha)
			{				
				alpha = max;

				if (alpha >= beta)
				{
					// ���J�b�g���ꂽalpha�Ƃ����l�͐��m�Ȓl�ł͂Ȃ����A���̃m�[�h�̕]���l�͍Œ�ł�alpha�ȏゾ�Ƃ������Ƃ��킩��B
					// �Ȃ̂ŁAalpha�͂��̃m�[�h�̉����l�ł���B������BOUND_LOWER
					break;
				}				
			}
		}
	}

	self.nextMinus();

	// �ő�ł��]���l��max�܂ł����s���Ȃ�
	tte->save(key, best_[depth].get(), max, remain_depth, TT.generation(),
		max < beta ? BOUND_EXACT : BOUND_LOWER, self.player(), my_remain_time, self.ojama() - enemy.ojama());

	return max;
}

Score AI::evaluateEX(LightField &self, LightField &enemy, int depth, int my_remain_time)
{
	eval_called++;

	Score score = self.evaluate(enemy, my_remain_time);

	if (score >= SCORE_MATE)
		score = SCORE_MATE;
	else if (score <= -SCORE_MATE)
		score = -SCORE_MATE;

	return score;
}