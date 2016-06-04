#include "AI.h"
#include "tt.h"
#include <cassert>
#include <algorithm>
#define HASH
unsigned int hash_hit;

int AI::thinkWrapper(const Field &self, const Field &enemy)
{
	int timeLimit = 999999;

	if (enemy.flag(RENSA))// �G���A����
	{
		// �A�����N���肫��܂ł̎��ԂƎv���΂悢
		timeLimit = (enemy.chainMax() - enemy.chain()) * (CHAINTIME + FALLTIME);
	}
	LightField s(self);
	LightField e(enemy);

	// �����i�߂�
	TT.newSearch();

	hash_hit = 0;

	// �v�l�J�n
	think(s, e, 0, timeLimit);

	// �x�X�g�Ȏ��best_[0]�ɂ͂����Ă���
	// ����𑀍�ɕϊ�
	if (easy_ == 0)
	{
		operate_.generate(best_[0], self);
	}
	else if (easy_ == 1)
	{
		operate_.easyGenerate(best_[0], self);
	}
	else if (easy_ == 2)
	{
		operate_.veryEasyGenerate(best_[0], self);
	}
	
	return 0;
}


// ��{�v�l���[�`��::depthMax��ǂ�
int AI::think(LightField &self, LightField &enemy, int depth, int timeLimit)
{
#ifdef HASH
	const int remain_depth = depth_max_ - depth;

	// ����ǖʂ���������̂�2��ڈȍ~�������肦�Ȃ��B
	if (depth >= 2)
	{
		const TTEntry *tte = TT.probe(self.key());
		// �ǖʂ��o�^����Ă���
		if (tte)
		{
			// �ǖʕ\�ɓo�^����Ă���ǖʂ��A���݂̋ǖʂ̎c��[���ȏ�
			if (tte->remainDepth() >= remain_depth)
			{
				best_[depth] = tte->move();

				// �ǖʕ\�̃X�R�A�͐M�p�ł���̂ł��̂܂ܕԂ�
				return tte->score();
			}
		}
	}
#endif

	if (depth >= depth_max_recieve_)
		return evalate(self, enemy, depth, timeLimit);
	else
	{	
		int score;
		int max = SCORE_LOSE - 1;
		int movenum;
		int con_prev[3];
		self.saveConnect(con_prev);
		Move move[22];// ������ꏊ�͍ő�łQ�Q��ނȂ̂�

		// �u���ꏊ�Ȃ� == ����
		if ((movenum = self.generateMoves(move)) == 0)
			return SCORE_LOSE;
		
#if defined(DEBUG)
		LightField f = self;// debug
#endif

		self.nextPlus();

		for (int i = 0; i < movenum; i++)
		{
			// generateMoves�ł��Ƃ߂��ꏊ�ɂ����Ă݂�
			self.doMove<true>(move[i]);
			assert(self.key() == self.keyInit());
			// �ݒu�ɂ����鎞�Ԃ��v�Z
			// �����ɂ����鎞�Ԃ́A13 - �ݒu�ʒu��y�̏������ق� ������Ȃ�A�]�v�Ɏ��Ԃ�������
			int takeTime = (13 - std::min(toY(move[i].csq()), toY(move[i].psq()))) * FALLTIME + (move[i].isTigiri() ? TAKETIME_INCLUDETIGIRI : TAKETIME);

			if (self.flag(VANISH))// ������Ȃ�
			{
				score = evalVanish(self, enemy, depth, timeLimit) - takeTime;
				self.clearFlag(VANISH);
			}
			else// �����Ȃ��Ƃ�
			{
				if (timeLimit < takeTime)
				{
					// �v���ʐU��Ƃ��ɔ��΂ł��Ȃ���Ε����B
					if (enemy.scoreMax() >= 30 * RATE || enemy.scoreMax() >= self.deadLine() * RATE) 
						score = SCORE_LOSE;
					else 
						score = think(self, enemy, depth + 1, timeLimit - takeTime) - takeTime;
				}
				else
					score = think(self, enemy, depth + 1, timeLimit - takeTime) - takeTime;
			}

			self.undoMove(move[i], con_prev);

			if (max < score)
			{
				max = score;
				best_[depth] = move[i];// �����Ƃ��]���̍������I��
			}

#if defined(DEBUG)
			assert(self == f); // debug::������Ƃ��Ƃ̋ǖʂɖ߂��Ă��邩
#endif
		}

		self.nextMinus();

#ifdef HASH
		// ���̒T���ł͌���̈����͎g��Ȃ�
		TT.store(self.key(), max, remain_depth, best_[depth], BOUND_EXACT, 0, 0, 0);
#endif
		return max;
	}
}

// �A�����N����Ȃ��Ƃ��̕]���֐�
int AI3Connect::evalNoVanish(const LightField &self, const LightField &enemy, int depth, int timeLimit) const 
{
	eval_called++;
	int score = 0;

	if (timeLimit < 900000 && timeLimit > 0 && enemy.scoreMax() >= self.deadLine())
		score -= 20000 / timeLimit;

	score += self.connectBonus(-1, 0, 1);
	score += self.positionBonus();
	
	return score;
}

// �A�����N����Ƃ��̕]���֐�
int AI3Connect::evalVanish(LightField self, const LightField &enemy, int depth, int timeLimit)
{
	eval_called++;
	int score = 0;

	while(self.deleteMin());

	if (self.isDeath())
		return -999999;

	if (timeLimit - self.chain() * CHAINTIME <= 0)
	{
		if (enemy.scoreMax() - self.score() > self.deadLine() * (int)RATE)// �v���ʂ�Ԃ�����Ȃ��Ȃ畉��
			score = -100000 * (depth_max_recieve_ - depth);
	}

	if (self.isEmpty())// �S����
		score += RATE * 30;

	if (self.score() < RATE * 30)// 1��U��Ȃ��������̓}�C�i�X�]��
		score -= RATE * 29;

	score += self.score() - enemy.scoreMax();

	if (timeLimit < 900000 && timeLimit > 0 && enemy.scoreMax() >= self.deadLine() * (int)RATE)
		score -= 20000 / timeLimit;

	score += self.connectBonus(-6, 3, 6);
	score += self.positionBonus();
	
	return score;
}

int AI3Connect2::evalNoVanish(const LightField &self, const LightField &enemy, int depth, int timeLimit) const 
{
	eval_called++;
	int score = 0;

	if (timeLimit < 900000 && timeLimit > 0 && enemy.scoreMax() >= self.deadLine() * (int)RATE)
		score -= 20000 / timeLimit;

	score += self.connectBonus(-6, 3, 6) / 2;
	//score += self.positionBonus();
	for (int x = 1; x <= 6; x++)
	{
		for (int y = 7; y < self.upper(x); y++)
		{ 
			if (x != 3)
			{
				if (!self.isEmpty(toSquare(x, y)))
					score -= RATE * y / abs(3-x);
			}
			else
			{
				if (!self.isEmpty(toSquare(x, y)))
					score -= RATE * y * 3;
			}
		}
	}
	return score;
}

int AI3Connect2::evalVanish(LightField self, const LightField &enemy, int depth, int timeLimit)
{
	eval_called++;
	int score = 0;

	const int self_num = self.getPuyoNum();
	while (self.deleteMin());

	if (self.isDeath())
		return SCORE_LOSE;

	if (timeLimit < 0)
	{
		if (enemy.scoreMax() - self.score() > self.deadLine() * (int)RATE)// �v���ʂ�Ԃ�����Ȃ��Ȃ畉
			return -999999;
		else
			score += self.score() - enemy.scoreMax();
	}

	if (self.isEmpty())// �S����
	{
		score += RATE * 30;
	}

	//if (self.score() < RATE * 20)// 1��U��Ȃ��������̓}�C�i�X�]��
	if (self.score() < RATE * 30)// 1��U��Ȃ��������̓}�C�i�X�]��
		score -= RATE * 29;
	else
		score += self.score();
		//score += s;

	score += self.connectBonus(-6, 3, 6) / 2;
	//score += self.positionBonus();
	for (int x = 1; x <= 6; x++)
	{
		for (int y = 7; y < self.upper(x); y++)
		{ 
			if (x != 3)
			{
				if (!self.isEmpty(toSquare(x, y)))
					score -= RATE * y / abs(3-x);
			}
			else
			{
				if (!self.isEmpty(toSquare(x, y)))
					score -= RATE * y * 3;
			}
		}
	}
	return score;
}


