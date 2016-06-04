#include "field.h"
#include "move.h"
#include "bitboard.h"
#include "chain.h"
#include "tt.h"
Color LightField::use_color_[4];

//#define DEBUG

// �t�B�[���h����ő�̘A���������C���̃X�R�A��Ԃ��D�����߂̃`�F�b�N�ƊÂ߂̃`�F�b�N������C���̊֐��͂�����Ăт킯�郉�b�p�[�ł���D
// �߂�l�͂��ז��Ղ扽���Ƃ����l�ɂ͕ϊ����Ă��Ȃ��D
Score LightField::maxChainsScore(const int ply_max, const int remain_time)
{
	chain_called++;

	// �O��ȑO�̘A���̏����N���A����B���p���邱�Ƃ��ł��邩������Ȃ��B
	chainsList()->clear();
	chainsList()->push_back(Chains(0, 0, 0));

	const uint64_t key = this->key();
	ChainsInfo* entry = CIT[key];
	Score score;

	if (entry->key == (key >> 32))
	{
		chain_hit++;

		*chainsList() = entry->cl;

		if (field_puyo_num_ > 42 || ((ojama_ >= 12 || ojama_ >= deadLine()) && (remain_time > 0 && remain_time <= ONEPUT_TIME * 3)))
		{
			// 2��ȓ��ɑłĂ�ő�̘A��
			auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
				[](Chains& a, Chains& b) { return a.ojama < b.ojama; });
			score = chain_max->ojama * RATE;
		}
		else
		{
			score = entry->max_score;
		}
	}
	else
	{
		// �� 1, 2�肵���ς߂Ȃ��̂Ȃ�A���ۂ̃c�������ŘA�����V�~�����[�g����
		if (field_puyo_num_ > 42 || ((ojama_ >= 12 || ojama_ >= deadLine()) && (remain_time > 0 && remain_time <= ONEPUT_TIME * 3)))
		{
			score = searchChains(3);

			if (score == SCORE_LOSE)
				return SCORE_LOSE;
		}
		else
		{
			// 2��ȓ��ɑłĂ�A���̃��X�g���\�z����D
			const Score chains_score = searchChains(3);

			if (chains_score == SCORE_LOSE)
				return SCORE_LOSE;

			// �����ɂƂ��Ēv���ʂ̘A����ł���āC����3�肮�炢�����u���Ȃ��ꍇ�́C�ő�������ڂ̃`�F�b�N������D
			if (ojama_ > deadLine() && remain_time < ONEPUT_TIME * 5)
			{
				// 2�ȏ�Ȃ����Ă���Ղ�������D3��łق����F��2������\����56%
				score = std::max(chains_score, chainBonus(2));
			}
			else
			{
				// �A����ł���Ă��Ȃ��ꍇ��4��ȏ�]�T������Ȃ�C�F�⊮�����ĘA�������邩�ǂ������ׂ�D
				// 2��ȏ�U��A���𑊎�ɑł���Ă��Ď��Ԃ����܂�Ȃ��Ȃ�C�F�⊮�͈�����Œ��ׂ�D
				const int help_max = 2;// ojama_ > 12 ? 1 : 2;

				// �Ղ�Ղ悪8��ȏ�ς܂�Ă���Ȃ�C�A�������������������������D
				if (field_puyo_num_ + field_ojama_num_ > 42 && flag(PLAYER_AI))
					score = std::max(chains_score, chainBonus(1));
				else
					score = std::max(chains_score, colorHelper(help_max));
			}
		}
		
		entry->save(key, score, chainsList());
	}

	return score;
}

// searchChains���Ăяo�����߂̃��b�p�[����,�Ӗ���debug���邽�߂ƁA�����ڂ̂��߁B
Score LightField::searchChains(const int ply_max)
{
#if defined(DEBUG)
	LightField f(*this);
#endif

	const Score s = searchChains(ply_max, 0);

#if defined(DEBUG)
	assert(f == *this);
#endif
	return s;
}

// ply_max�肩���ċN������A���̂�����ԑ傫�ȘA���̓_����Ԃ��B
// ply_max�肾���T�����ĘA�����N���邩�ǂ������V�~�����[�g����B
// �I�Ղɂ����Ă͂�����Ăяo���ق����悢�D�����łȂ��ꍇ�͈Ⴄ�֐��D
// �A���Ă���l�͂��ז������Ƃ������l�ł͂Ȃ��C���̓_���D
// const �֐��ł͂Ȃ��� const �֐��Ȃ͂��B
// TODO: �ł�����̌`�̗ǂ�(positionBonus())���L�����Ă����ׂ����H
Score LightField::searchChains(const int ply_max, int ply)
{
	static int taketime_stack[3] = { 0 };
	if (ply == ply_max)
	{
		// �����ɂ���ꍇ�͘A�����N���Ȃ������ꍇ�Ȃ̂ŁA������0��Ԃ��B
		return isDeath() ? SCORE_LOSE : SCORE_ZERO;
	}

	Move mlist[22];
	int movenum = generateMoves(mlist);
	Score best_score = SCORE_MIN - Score(1);
	Score s;

	// ���i�߁A�n�b�V���������v�Z����B
	nextPlus();

	// for debug
	const uint64_t w_key = key();

	for (int i = 0; i < movenum; i++)
	{
		doMove<false>(mlist[i]);

		// �ݒu�ɂ����鎞�Ԃ��v�Z
		// �����ɂ����鎞�Ԃ́A13 - �ݒu�ʒu��y�̏������ق� ������Ȃ�A�]�v�Ɏ��Ԃ�������
		taketime_stack[ply] = (13 - std::min(toY(mlist[i].csq()), toY(mlist[i].psq()))) * FALLTIME + (mlist[i].isTigiri() ? TAKETIME_INCLUDETIGIRI : TAKETIME);

		if (flag(VANISH))
		{
			// ������Ȃ�_�����v�Z����B
			s = deleteScore();

			// ���ז�4�ȏ�U��Ȃ�A���Ƃ��ĔF�߂�D
			if (s > 4 * RATE)
			{
				int take_frame = 0;
				for (int y = 0; y <= ply; y++)
					take_frame += taketime_stack[y];
				// �A���Ŕ������邨�ז��Ղ搔�A�A���ɂ�����t���[�����A���΂ɂ�����t���[��
				Chains entry = Chains(s / RATE, CHAINTIME * chainMax(), take_frame);

				// �����o�^�ς݂̘A���Ȃ�o�^���Ȃ��D
				auto iter = std::find_if(chainsList()->begin(), chainsList()->end(), [&](Chains &c) { return entry == c; });

				if (iter == chainsList()->end())
					chainsList()->push_back(entry);
			}

			clearFlag(VANISH);
		}
		else
			s = searchChains(ply_max, ply + 1);

		undoMove(mlist[i]);

		assert(w_key == key());

		if (s > best_score)
			best_score = s;
	}

	nextMinus();

	return best_score;
}

// �\�ʂɘI�o���Ă���Ղ�𑍓���ŏ����C��ԑ傫�ȘA���̃X�R�A��Ԃ��D
// connect_num �ȏ�A�����Ă���ꏊ���������Ȃ��B
Score LightField::chainBonus(const int connect_num) const 
{
	assert(recul(0) == RANK_MAX && recul(7) == RANK_MAX);

	int left, right;
	setRange2(&left, &right);

	Score max = SCORE_ZERO, s = SCORE_ZERO;

	// �I�o���Ă���Ղ������
	for (int x = left; x <= right; x++)
	{
		assert(x >= 1 && x <= 6);
		assert(isInSquare(x, upper(x) - 1));

		// ������܂Ղ���������Ƃ������B
		for (int y = upper(x) - 1; y >= 1; y--)
		{
			const Square sq = toSquare(x, y);

			// �I�o���Ă�������𒲂ׂ�
			Connect c = CON_NONE;

			if (isEmpty(sq + SQ_UP))
				c |= CON_UP;
			if (isEmpty(sq + SQ_RIGHT))
				c |= CON_RIGHT;
			if (isEmpty(sq + SQ_LEFT))
				c |= CON_LEFT;

			// �I�o���Ă��Ȃ� == �����艺�̂Ղ���I�o���Ă��Ȃ�
			if (c == CON_NONE)
				break;

			const Color cl = color(sq);

			// 2��ȏ㓯�������������Ȃ����߂ɗ���������Ă���
			Bitboard bb_history[4];

			Bitboard bb = bbSameColor(sq);

			for (int i = 0; c; c &= c - 1)
			{
				Bitboard bb_same(bb);
				const Square open_sq = sq + lsbToPosition(c);

				if (color(open_sq + SQ_RIGHT) == cl && open_sq + SQ_RIGHT != sq)
				{
					bb_same |= bbSameColor(open_sq + SQ_RIGHT);
				}
				if (color(open_sq + SQ_DOWN) == cl && open_sq + SQ_DOWN != sq)
				{
					bb_same |= bbSameColor(open_sq + SQ_DOWN);
				}
				if (color(open_sq + SQ_LEFT) == cl && open_sq + SQ_LEFT != sq)
				{
					bb_same |= bbSameColor(open_sq + SQ_LEFT);
				}

				bool cont = false;

				// �ŏ���i��0�Ȃ̂ł���for�ɂ͓���Ȃ��B
				for (int j = 0; j < i; j++)
				{
					if (bb_history[j] == bb_same)
					{
						cont = true;
						break;
					}
				}

				if (cont)
					continue;

				bb_history[i++] = bb_same;

				const int count = bb_same.count();

				if (count >= connect_num)
				{
					LightField f(*this);

					assert(f.puyo(sq) != EMPTY);

					if (f.puyo(sq) == OJAMA)
					{
						f.hash_key_ ^= f.hash_->seed(sq, OJAMA);
						f.field_[sq] = EMPTY;
						f.recul_y_[toX(sq)] = toY(sq);
					}
					else
					{
						f.deletePuyo<true>(bb_same);
					}

					// �X���C�h�͔������Ȃ�
					if (!f.clearConnect())
						continue;

					f.slideMin<true>();
					f.setConnectRecul();

					assert(f.key() == f.keyInit());

					f.chain_ = 1;

					s = f.deleteScoreDestroy() * (count + 7) / 10;
					
					if (max < s)
						max = s;
				}
			}
		}
	}

	return max;
}

// �t�B�[���h�̒u����ꏊ�ɐF��⊮���ĘA�������邩�ǂ������ׂ�B
// �⊮������̍ő傪help_max
// debug : const �֐��ɂȂ��Ă��Ȃ����assert����B
Score LightField::colorHelper(const int help_max)
{	
	Score max = SCORE_ZERO;

	for (int i = 0; i < help_max; i++)
	{
#if defined (DEBUG)
		LightField f(*this);
#endif
		const Score s = colorHelper(i, 0);

#if defined (DEBUG)
		assert(f == *this);
#endif
		if (s > max)
			max = s;
	}

	return max;
}

// �t�B�[���h�̒u����ꏊ�ɐF��⊮���ĘA�������邩�ǂ������ׂ�B
// �⊮������̍ő傪help_max�@const �֐��ł͂Ȃ��� const �֐��Ȃ͂��B
Score LightField::colorHelper(const int help_max, int help_num)
{
	// �c��⊮��
	const int remain_help = help_max - help_num;
	int right, left;
	setRange(&left, &right);
	
	Score max = SCORE_ZERO;
	
	for (int i = 0; i < 4; i++)
	{
		const Color c = LightField::useColor(i);

		for (int x = left; x <= right; x++)
		{
			color_searched++;
			const Square sq = toSquare(x, upper(x));

			if(sq == DEADPOINT)
				continue;

			// do
			field_[sq] = c;
			setConnectMin(sq);
			
			if (flag(VANISH))
			{
				clearFlag(VANISH);
				resetConnect(sq);
				field_[sq] = EMPTY;
				continue;
			}

			++upper_y_[x];
			hash_key_ ^= hash_->seed(sq, c);

			Score s;
			const uint64_t key = this->key() + remain_help;

			// �ǖʕ\�����ĒT���ς݂Ȃ�l�𓾂�continue.
			EvaluateHashEntry entry = *ET[key];

			if (entry.key() == (key >> 32) && entry.remainHelp() == remain_help)
			{
				++allready_searched;
				s = entry.score();
			}
			else
			{
				// �������ő�[���ł���
				if (help_num >= help_max)
				{	
					s = chainBonus(1);
				}
				else
				{
					s = colorHelper(help_max, help_num + 1);
				}
				
				// �G���g���[�ɓo�^
				entry.save(key, s, remain_help);
				*ET[key] = entry;
			}

			// undo
			hash_key_ ^= hash_->seed(sq, c);
			resetConnect(sq);
			field_[sq] = EMPTY;
			--upper_y_[x];

			if (s > max)
				max = s;
		}
	}
	return max;
}

int LightField::examine() const // Field�̐��������`�F�b�N����
{
	uint64_t hash_key_w = 0;
	int con_w[3] = { 0, 0, 0 };
	Bitboard bb_same(0, 0);
	int tumo_num = 0, obstacle_num = 0, pos_bonus = 0;

	for (int x = 0; x < FILE_MAX; x++)
	{
		for (int y = 0; y < 15; y++)
		{
			const Square sq = static_cast<Square>(x * 16 + y);

			if (x >= 1 && y >= 1 && x <= 6 && y <= 13)
			{
				if (color(sq) != EMPTY)
				{
					if (color(sq) != OJAMA)
					{
						tumo_num++;
						pos_bonus += Bonus::position_bonus_seed[sq];
					}
					else
					{
						obstacle_num++;
						pos_bonus += Bonus::position_ojama_penalty[sq];
					}
				}
			}
			if (x == 0 || x == 7 || y == 0)
			{
				if (color(sq) != WALL)
					assert(false);
			}
			if (color(sq) == OJAMA && connect(sq))
				assert(false);

			if (connect(sq))// �ǂ����̂Ղ�ƂȂ����Ă���ꍇ
			{
				if (color(sq) == EMPTY || color(sq) == OJAMA || color(sq) == WALL)
					assert(false);

				for (Connect c = connect(sq); c; c &= c - 1)
				{
					// �Ȃ����Ă���Ղ摤���猩�āA�����̕����ɂȂ����Ă��Ȃ�
					// �܂��́A�����Ɠ����F�ł͂Ȃ�
					const Rotate r = lsbToRotate(c);
					const Square pos = sq + rotatePosition(r);

					if (!(connect(pos) & toDirRev(r)) || (color(sq)) != color(pos))
						assert(false);
				}
			}
		}
	}

	for (int x = 1; x <= 6; x++)
	{
		for (int y = 1; y <= 13; y++)
		{
			const Square sq = toSquare(x, y);

			if (!isEmpty(sq))
				hash_key_w ^= hash_->seed(sq, color(sq));

			if (!flag(VANISH))
			{
				if (bb_same.isSet(sq) || color(sq) == EMPTY || color(sq) == OJAMA)
				{
					continue;
				}
				else
				{
					// �����Ղ悪�u����Ă�����
					int count = findSameColor(bb_same, sq);					
					assert(count < 4);
					con_w[count - 1]++;
				}
			}
			if (isEmpty(sq) && !isEmpty(sq + SQ_DOWN))
			{
				if (upper(x) != y)
				{
					assert(false);
				}
				else
				{
					// ��������EMTPY���������炾��
					for (int d = 1; y - d >= 1; d++)
					{
						if (isEmpty(toSquare(x, y - d)))
							assert(false);
					}

					// ��������EMPTY�ł͂Ȃ��Ƃ��낪�������炾��
					for (int dy = y; dy < 15; dy++)
					{
						if (!isEmpty(toSquare(x, dy)))
							assert(false);
					}
					break;
				}
			}
		}
	}

	hash_key_w ^= hash_->seed(tumoIndex());

	if (flag(ALLCLEAR))
		hash_key_w ^= hash_->seed();

	if (!flag(VANISH))
		for (int i = 0; i < 3; i++)
			if (connect_[i] != con_w[i])
			{
				LightField f(*this);
				f.bonusInit();
				assert(f.examine());
			}

	assert(hash_key_ == hash_key_w);

	if (tumo_num != field_puyo_num_
		|| obstacle_num != field_ojama_num_
		|| pos_bonus != position_bonus_)
	{
		assert(false);
	}
	return 1;
}

// AI�p�ɒ�`
LightField::LightField(const Field &f)
{
	// field����K�v�ȃ����o�����R�s�[
	memcpy(this, &f, sizeof(LightField));
}


// �t�B�[���h��ɓ����F��T���B
// �����F�ɂȂ����Ƃ����1�ɂ���B
int LightField::findSameColor(Bitboard& bb, const Square sq) const
{
	bb.set(sq);
	int con_num = 1;

	for (Connect c = connect(sq); c; c &= c - 1)
	{
		const Square next_sq = sq + lsbToPosition(c);

		if (bb.isNotSet(next_sq))
			con_num += findSameColor(bb, next_sq);
	}

	return con_num;
}

// bitboard ���Z�b�g���邾��
void LightField::setSameColor(Bitboard & bb, const Square sq) const
{
	bb.set(sq);

	for (Connect c = connect(sq); c; c &= c - 1)
	{
		const Square next_sq = sq + lsbToPosition(c);

		if (bb.isNotSet(next_sq))
			setSameColor(bb, next_sq);
	}
}

// sq���瓯���F�łȂ����Ă���ꏊ��1�ɂȂ��Ă���bitboard��Ԃ��B
Bitboard LightField::bbSameColor(const Square sq) const
{
	Bitboard bb(0, 0);
	setSameColor(bb, sq);
	return bb;
}

// field_[x][y]�̎���ɂȂ����Ă���Ղ悪�������ꍇ�A
// field_[x][y]�������ہA����̃R�l�N�g��񂪎c���Ă��܂�����,�܂��̃R�l�N�g�����ɖ߂�
void LightField::resetConnect(const Square sq)
{
	assert(color(sq) != EMPTY && color(sq) != OJAMA);

	// 13�i�ڂ�connect���Ȃ�
	if (toY(sq) >= 13)
		return;

	for (Connect c = connect(sq); c; c &= c - 1)
	{
		// ����
		const Rotate r = lsbToRotate(c);

		field_[sq + rotatePosition(r)] &= ~toDirRev(r);
	}
}

// ���@�萶��
int LightField::generateMoves(Move* move) const
{
	const Move* begin = move;

	// 3,12���l�܂��Ă���Ƃ��͎���ł���ǖ�
	if (upper_y_[3] >= RANK_13)
		return 0;

	int right, left;
	setRange(&left, &right);

	const Tumo now = getNowTumo();
	const bool color_is_diff = (now.pColor() != now.cColor());

	for (int x = right; x >= left; x--)
	{
		const int up = upper(x);
		const int up_side = upper_y_[x + 1];
		const Square dest = toSquare(x, up);
		const Square dest_up = dest + SQ_UP;

		assert(isEmpty(dest) && !isEmpty(dest + SQ_DOWN));

		// ��]��0�ł�����Ȃ�2�ł��u���邵�A1�ł�����Ȃ�3�ł��u����
		if (color_is_diff)
		{
			if (up_side < RANK_13)
			{
				const Square dest_side = toSquare(x + 1, up_side);
				const bool is_tigiri = (up != up_side);

				// 4���C�ɐ����B
				*(uint64_t*)move = (uint64_t)dest_up << 56 | (uint64_t)dest << 48				   // rotate == 0
					| (uint64_t)dest << 40 | (uint64_t)dest_up << 32							   // rotate == 2
					| (uint64_t)is_tigiri << 31 | (uint64_t)dest_side << 24 | (uint64_t)dest << 16 // rotate == 1
					| (uint64_t)is_tigiri << 15 | (uint64_t)dest << 8 | (uint64_t)dest_side << 0;  // rotate == 3
				move += 4;
			}
			else
			{
				// 2�肸�����B
				*(uint32_t*)move = dest_up << 24 | dest << 16
					| dest << 8 | dest_up << 0;
				move += 2;
			}
		}
		else
		{
			if (up_side < RANK_13)
			{
				const Square dest_side = toSquare(x + 1, up_side);
				const bool is_tigiri = (up != up_side);

				// 2�肸�����B
				*(uint32_t*)move = dest_up << 24 | dest << 16
					| is_tigiri << 15 | dest_side << 8 | dest << 0;
				move += 2;
			}
			else
			{
				// 1�肾�������B
				*(move++) = Move(dest, dest_up, false);
			}
		}

	}

	// ����������̐�
	ptrdiff_t move_count = move - begin;
	assert(move_count <= 22);

#if defined(DEBUG)
	if (right == 6 && left == 1)
		assert((color_is_diff && move_count == 22) || move_count == 11);

	for (int i = 0; i < move_count; i++)
		assert(isEmpty(begin[i].psq()) && isEmpty(begin[i].csq()));
#endif

	return (int)move_count;
}

// move������A�A���A������܂Ղ�̗���������Ȃ痎�����s��
int LightField::doMoveEX(const Move &move, int my_remain_time, LightField& enemy)
{
	const Square psq = move.psq();
	const Square csq = move.csq();
	const Tumo now = getPreviousTumo();
	const Color pc = now.pColor();
	const Color cc = now.cColor();

	field_[psq] = pc;
	setConnect(psq);

	field_[csq] = cc;
	setConnect(csq);

	++upper_y_[toX(psq)];
	++upper_y_[toX(csq)];

	position_bonus_ += Bonus::position_bonus_seed[psq];
	position_bonus_ += Bonus::position_bonus_seed[csq];

	// �����v�Z�ł���̂ł���
	hash_key_ ^= hash_->seed(psq, pc);
	hash_key_ ^= hash_->seed(csq, cc);

	field_puyo_num_ += 2;

	// ��ɂ����鎞�Ԃ��v�Z���A�����̎c�莞�Ԃ������
	int fall_time = (12 - std::min(toY(csq), toY(psq)));

	// �����{�[�i�X
	score_ += fall_time;

	// ����11�i�ڂ���ɒu����Ȃ�Ax�����̈ړ������Ԃɓ����B
	if (fall_time <= FALLTIME)
		fall_time += abs(3 - toX(psq));

	assert(!(ojama_ == 0 && flag(OJAMA_WILLFALL)));

	const bool vanish = flag(VANISH);

	if (vanish)
	{
		clearFlag(VANISH);

		// �A�����I���܂ŏ����B���̃��\�b�h�ŏ�������̃R�l�N�g���͍Čv�Z����Ă���
		while (deleteMin());

		if (isAllClear())
		{
			setFlag(ALLCLEAR);
			hash_key_ ^= hash_->seed();
		}

		// �R�l�N�g���̍Čv�Z
		if (bonusInit() == -1)
			assert(false);

		// ���E
		offseting(enemy);

		// ��������ɕԂ���������A���ɑ���ɂ�����܂Ղ悪�~��t���O�𗧂ĂĂ���
		if (enemy.ojama_)
			enemy.setFlag(OJAMA_WILLFALL);
	}

	int put_time = fall_time * FALLTIME									// ��������
		+ (move.isTigiri() ? TAKETIME_INCLUDETIGIRI : TAKETIME)			// �d�����ԁB������Ȃ�A�ǉ�����
		+ (vanish ? CHAINTIME * chainMax() + NEXTTIME : 0);				// �A���Ȃ�A�A������ + �d������

	chain_max_ = 0;

	// �����ŁA�����ɂ��ז��Ղ悪�~��Ȃ�~�点��
	// ���ז��Ղ悪�U�鎞�ԂƁA���肪�s���\�ɂȂ�܂ł̎��Ԃ͏����Ⴄ�B
	// �A����̍d�����Ԃ��������l�����ז��Ղ悪�U��܂ł̃^�C�����~�b�g
	if (flag(OJAMA_WILLFALL) && my_remain_time - put_time - TAKETIME - 1 <= 0)
	{
		assert(ojama_ != 0);

		ojamaFallMin(&ojama_);

		if (ojama_ == 0)
			clearFlag(OJAMA_WILLFALL);

		// ���ז��Ղ悪�U�����Ƃ��́A�X�ɍd�����Ԃ�����B
		put_time += NEXTTIME;
	}

	assert(examine());
	return put_time;
}

// �ǖʂ�߂�
void LightField::undoMove(const Move &move, int *con_prev)
{
	connect_[0] = con_prev[0];
	connect_[1] = con_prev[1];
	connect_[2] = con_prev[2];
	const Square psq = move.psq();
	const Square csq = move.csq();
	const Tumo now = getPreviousTumo();
	const Color pc = now.pColor();
	const Color cc = now.cColor();
	--upper_y_[toX(psq)];
	--upper_y_[toX(csq)];
	resetConnect(psq);
	resetConnect(csq);
	hash_key_ ^= hash_->seed(psq, pc);
	hash_key_ ^= hash_->seed(csq, cc);
	field_[psq] = EMPTY;
	field_[csq] = EMPTY;
	position_bonus_ -= Bonus::position_bonus_seed[psq];
	position_bonus_ -= Bonus::position_bonus_seed[csq];	
}

// �{�[�i�X�̌v�Z�����Ȃ�
void LightField::undoMove(const Move &move)
{
	const Square psq = move.psq();
	const Square csq = move.csq();
	--upper_y_[toX(psq)];
	--upper_y_[toX(csq)];
	resetConnect(psq);
	resetConnect(csq);
	hash_key_ ^= hash_->seed(psq, color(psq));
	hash_key_ ^= hash_->seed(csq, color(csq));
	field_[psq] = EMPTY;
	field_[csq] = EMPTY;	
}

// ���ז��Ղ悪�~��Ƃ��A������Ăяo��
void LightField::ojamaFallMin(int *ojama)
{
	int rand_ojama[6] = { 1, 2, 3, 4, 5, 6 };

	int now_fall_ojama;

	if (*ojama >= 30)
	{
		now_fall_ojama = 30;
		*ojama -= 30;
	}
	else
	{
		now_fall_ojama = *ojama;
		*ojama = 0;

		// �V���b�t��
		for (int n = 0; n < 6; n++)
		{
			int r = ojama_pool_[ojama_rand_id_];
			ojama_rand_id_ = (ojama_rand_id_ + 1) & 127;
			std::swap(rand_ojama[n], rand_ojama[r]);
		}
	}

	// ������܂Ղ悪6�ȉ��ɂȂ�܂ł́C�����珇�Ԃɂ��ז��Ղ����ׂĂ���
	for (int x = 1; now_fall_ojama > 6;)
	{
		const int y = upper(x);
		const Square sq = toSquare(x, y);

		if (y <= RANK_13)
		{
			assert(isEmpty(sq));
			field_[sq] = OJAMA;
			hash_key_ ^= hash_->seed(sq, OJAMA);
			position_bonus_ += Bonus::position_ojama_penalty[sq];
			++field_ojama_num_;
			++upper_y_[x];
		}
		--now_fall_ojama;

		if (++x > 6)
			x = 1;
	}

	// 6�ȉ��ɂȂ�����C�����_���ɒu���D
	for (int x = 0; now_fall_ojama > 0; x++)
	{
		const int y = upper(rand_ojama[x]);
		const Square sq = toSquare(rand_ojama[x], y);

		if (y <= RANK_13)
		{
			assert(isEmpty(sq));
			field_[sq] = OJAMA;
			hash_key_ ^= hash_->seed(sq, OJAMA);
			position_bonus_ += Bonus::position_ojama_penalty[sq];
			++field_ojama_num_;
			++upper_y_[rand_ojama[x]];
		}
		--now_fall_ojama;
	}
}

// ���E����
void LightField::offseting(LightField& enemy)
{
	int send_ojama_num = score_ / RATE;

	if (send_ojama_num)
	{
		if (ojama_)// ���������ɂ��ז����ӂ肻���Ȃ�
		{
			ojama_ -= send_ojama_num;// ���E

			if (ojama_ < 0)
			{
				enemy.ojama_ += -ojama_;// �}�C�i�X�ɂȂ������Ɋւ��ẮA����ɂ�����
				ojama_ = 0;
				clearFlag(OJAMA_WILLFALL);
			}
			else if (ojama_ == 0)
				clearFlag(OJAMA_WILLFALL);
		}
		else
		{
			enemy.ojama_ += send_ojama_num;// ������܂Ղ��������
			assert(!flag(OJAMA_WILLFALL));
		}

		score_ %= RATE;
	}
}

// �ǖʂ̃n�b�V���l�̃Z�b�g�B
// �����v�Z�͂����A�����ň�C�ɂ��B
uint64_t LightField::keyInit()
{
	hash_key_ = 0;

	for (int x = 1; x <= 6; x++)
	{
		for (int y = 1; y < upper(x); y++)
		{
			const Square sq = toSquare(x, y);

			if (color(sq) != EMPTY)
				hash_key_ ^= hash_->seed(sq, color(sq));
		}
	}

	hash_key_ ^= hash_->seed(tumoIndex());

	if (flag(ALLCLEAR))
		hash_key_ ^= hash_->seed();

	return hash_key_;
}

void LightField::aiInit()
{
	keyInit();
	bonusInit();
}


// �A����A�R�l�N�g���̍Čv�Z���s��
// �����Ńc�����������Ă���
// ���łɃ{�[�i�X�_�̌v�Z���s��
int LightField::bonusInit()
{
	Bitboard bb_same(0, 0);
	connect_[0] = connect_[1] = connect_[2] = 0;
	field_puyo_num_ = 0;
	field_ojama_num_ = 0;
	position_bonus_ = 0;

	for (int x = 1; x <= 6; x++)
	{
		for (int y = 1; y < upper(x); y++)
		{
			const Square sq = toSquare(x, y);

			if (color(sq) == OJAMA)
			{
				field_ojama_num_++;
				position_bonus_ += Bonus::position_ojama_penalty[sq];
			}
			else
			{
				assert(!isEmpty(sq));
				field_puyo_num_++;
				position_bonus_ += Bonus::position_bonus_seed[sq];
			}

			if (bb_same.isSet(sq) || field_[sq] == OJAMA)
			{
				assert(!isEmpty(sq));
				continue;
			}
			else
			{
				// �����Ղ悪�u����Ă�����
				int count = findSameColor(bb_same, sq);
				
				assert(count < 4);
				connect_[count - 1]++;
			}
		}
	}
	assert(connect_[0] + connect_[1] * 2 + connect_[2] * 3 == field_puyo_num_);
	return 0;
}

// �Čv�Z���K�v�Ȕ͈͂����Aconnect����clear����
// �����A����f���������������Ȃ�true��Ԃ��B����false�Ȃ�X���C�h�͔������Ȃ����Ƃ��킩��B
bool LightField::clearConnect()
{
	bool connect_clear = false;

	for (int x = 1; x <= 6; x++)
	{
		for (int y = recul(x); y < upper(x); y++)
		{
			const Square sq = toSquare(x, y);

			if (isEmpty(sq))
				continue;

			connect_clear = true;
			field_[sq + SQ_RIGHT] &= ~CON_LEFT;// �E�������猩��΁A���ɂȂ����Ă���̂�
			field_[sq + SQ_LEFT] &= ~CON_RIGHT;// ���������猩��΁A�E�ɂȂ����Ă���̂�
			field_[sq] &= 0xf0;
		}
	}

	return connect_clear;
}

// �Čv�Z���K�v�Ȕ͈͂����Aconnect����set����
void LightField::setConnectRecul()
{
	for (int x = 1; x <= 6; x++)
	{
		for (int y = recul(x); y < upper(x); y++)// 13�i�ڂ̂Ղ�̓R�l�N�g���Ă��Ȃ�
		{
			assert(y >= 0 && y <= 13);
			const Square sq = toSquare(x, y);

			if (y < RANK_13 && !isEmpty(sq) && puyo(sq) != OJAMA)
			{
				// ������𒲂ׂ�K�v�͂Ȃ��B�Ȃ��Ȃ���̂Ղ�̉������𒲂ׂĂȂ����Ă����
				// ��ɂȂ����Ă���͂�������B
				for (Rotate r = ROTATE_RIGHT; r < ROTATE_MAX; ++r)
				{
					const Square next_sq = sq + rotatePosition(r);

					if (color(sq) == color(next_sq))
					{
						field_[sq] |= toDir(r);
						field_[next_sq] |= toDirRev(r);
					}
				}
			}
		}
	}
}

// chain_ = �A����
// score_ = ���̘A���̓_��
// �R�l�N�g���̍Čv�Z���s���B�R�l�N�g���̍Čv�Z�͂��Ȃ��B
// �߂�l�F�A�����N����Ȃ��Ȃ�����false
bool LightField::deleteMin()
{
	// ���̋ǖʂ̃L�[
	const uint64_t key = this->key();

	// �A���e�[�u��������ɒ��ׂ��`�ł͂Ȃ����𒲂ׂ�B
	const ChainEntry* chain_entry = CT.probe(key);

	ChainElement ce(chain());
	Bitboard bb_over4;

	// ������Ȃ�true�ɂȂ�
	bool can_delete;

	// �o�^����Ă���`�ł���΁A������ꏊ�Ɠ_���v�Z�p�̒l���擾����
	if (chain_entry)
	{
		if (chain_entry->chainElement()->puyoNum())
		{
			can_delete = true;
			bb_over4 = chain_entry->bbDelete();
			ce.set(chain_entry->chainElement());	
		}
		else
		{
			// �A���͍Œ�ł�1�_�ȏ�͂���̂ŁA0�_�Ȃ�A�����N���Ȃ��`�ł���Ƃ������ƁB
			can_delete = false;
		}
	}
	else // �����łȂ���΁A�t�B�[���h�𑖍����Ē��ׂ�B
	{
		Bitboard bb_allready(0, 0);
		int color_bit = 0;
		bb_over4 = Bitboard(0, 0);

		if (chain_ == 0)
		{
			// �œK�������͂�
			for (int i = 1; i <= 6; i++)
				recul_y_[i] = RANK_2;
		}

		for (int x = 1; x <= 6; x++)
		{
			// �����ׂĂ���x��ڂƁA���ׂƉE�ׂ̍ŏ��l�Ԗڂ̒i���璲�ׂ�
			// int y = 1�Ə������͂����Ƒ���
			const int wy = min3(recul(x - 1), recul(x), recul(x + 1));
			assert(wy > 0);

			for (int y = wy - 1; y < upper(x); y++)
			{
				const Square sq = toSquare(x, y);
				const Connect c = connect(sq);

				// �Ȃ����Ă���Ղ悪2�ȉ�
				// �A����������Ƃ��́A�Œ�ł��ЂƂ͓�ȏ�Ȃ����Ă���Ղ悪���݂���
				if ((c & (c - 1)) && bb_allready.isNotSet(sq))
				{
					Bitboard bb = bbSameColor(sq);
					bb_allready |= bb;
					const int count = bb.count();

					if (count >= 4)
					{
						bb_over4 |= bb;
						color_bit |= 1 << colorToColorType(color(sq));
						ce.connectPlus(count);
					}
				}
			}
		}

		// �A�����N����Ȃ�A�Œ�ł��ЂƂ�color_bit�����B
		if (color_bit)
		{
			can_delete = true;
			ce.setColorBonus(Bitop::popCount(color_bit));
			ce.setPuyoNum(bb_over4.count());
		}
		else
			can_delete = false;
	}

	if (can_delete)// �A�����N�����ꍇ
	{
		if (chain_entry)
		{
			deletePuyo<false>(bb_over4);
		}
		else
		{
			deletePuyo<true>(bb_over4);
			assert(this->key() == this->keyInit());
		}

		int bonus = ce.chainScore();

		if (bonus == 0)
			bonus = 1;

		if (flag(ALLCLEAR))// �S�����Ȃ�
		{
			score_ += RATE * 30;
			clearFlag(ALLCLEAR);
			hash_key_ ^= hash_->seed();
		}

		chain_++;
		score_ += bonus;

		// connect���̍Čv�Z
		clearConnect();

		if (chain_entry)
		{
			// �X���C�h. �n�b�V���̍����v�Z�͂Ȃ�
			slideMin<false>();
		}
		else
		{
			// �n�b�V���̍����v�Z������
			slideMin<true>();
			assert(this->key() == this->keyInit());
		}

		setConnectRecul();

		if (chain_entry)
		{
			this->hash_key_ = chain_entry->afterKey();
		}
		else
		{
			// ���ɒ��ׂ�Ƃ��ɃL�[�l���������Ȃ���΂����Ȃ��B
			assert(this->key() == this->keyInit());

			// ���̋ǖʂ�o�^���Ă����B
			CT.store(key, this->key(), ce, bb_over4);
		}
	}
	else
	{
		// ���̋ǖʂ�o�^���Ă����B
		if (!chain_entry)
		{
			CT.store(key, this->key(), ce, bb_over4);
			assert(!bb_over4.isTrue());
		}
		chain_max_ = chain_;
		chain_ = 0;
	}
	return can_delete;
}

// �A���̓_��������m�肽���Ƃ��́A���ɒ��ׂ����Ƃ̂���ǖʂȂ炱���炪����
// �A���̓_�������m�肽���Ƃ��Ƃ����̂́A�]���֐����Ŏ����������Ă���A�����X�g���\�z����Ƃ���
// ���̘A���̓_�������킩��΂悭�A���ۂɋǖʂ�i�߂�K�v���Ȃ��ꍇ���w���B
// �����o�ϐ�chain_max_��ω�������̂�const�֐��ł͂Ȃ��B����������ȊO��const�֐��ł���B
Score LightField::deleteScore()
{
	chain_called++;

	// ���̋ǖʂ̃L�[
	const uint64_t key = this->key();

	// �A���e�[�u��������ɒ��ׂ��`�ł͂Ȃ����𒲂ׂ�B
	const ChainEntry* chain_entry = CT.probe(key);

	ChainElement ce(chain());

	// �r���̃G���g���[�����Ă��邩�D
	bool entry_deleted = false;

	// �o�^����Ă���`�ł���΁A������ꏊ�Ɠ_���v�Z�p�̒l���擾����
	if (chain_entry)
	{
		chain_hit++;

		Score bonus = SCORE_ZERO;

		// �A�����N���Ȃ��Ȃ�܂�
		while (chain_entry->chainElement()->puyoNum())
		{
			// �G���g���[����A���v�f�𓾂�B
			ce.set(chain_entry->chainElement());

			// �{�[�i�X�_���v�Z
			bonus += ce.chainScore();
			
			if (!bonus)
				bonus = Score(1);

			// x�A���ڂ͒��ׂ��̂ŁAx + 1�A���ڂɂ���
			ce.plusChain();

			// ���̋ǖʂɂ��Ē��ׂ���A���̋ǖʂ̃n�b�V���L�[�𓾂�probe����B
			chain_entry = CT.probe(chain_entry->afterKey());

			if (!chain_entry)
			{			
				// �r���̃G���g���[���󂳂�Ă����B���̎��͎d�����Ȃ��̂ŔՖʂ�ω�������B
				entry_deleted = true;
				break;
			}
		}

		if (!entry_deleted)
		{
			// �Ăяo�����ŉ��A���������̂���chainMax()���\�b�h�ŋ��߂���悤�ɁB
			chain_max_ = ce.chain();

			if(bonus && flag(ALLCLEAR))
				bonus += RATE * Score(30);

			return score() + bonus;
		}
	}
	
	// �A���e�[�u���ɓo�^����Ă��Ȃ����A�o�^����Ă��Ă��r���̃G���g���[�����Ă����ꍇ�͂����ɂ���B
	LightField f(*this);

	Score s = SCORE_ZERO;

	// 1�����
	if(f.deleteMin())
	{
		s = f.deleteScoreDestroy();
	}

	// �Ăяo�����ŉ��A���������̂���chainMax()���\�b�h�ŋ��߂���悤�ɁB
	chain_max_ = f.chainMax();
	return f.score();
}

// �ǖʂ��Ԃ����Ă������̂łƂɂ����A���X�R�A���ق����Ƃ��͂���̂ق��������D
// �ǖʂ̃R�s�[������ČĂяo���Ƃ��ɂ͂�����g���Ƃ����D
Score LightField::deleteScoreDestroy()
{
	// ���̋ǖʂ̃L�[
	const uint64_t key = this->key();

	// �A���e�[�u��������ɒ��ׂ��`�ł͂Ȃ����𒲂ׂ�B
	const ChainEntry* chain_entry = CT.probe(key);

	ChainElement ce(chain());

	// �r���̃G���g���[�����Ă��邩�D
	bool entry_deleted = false;

	// �o�^����Ă���`�ł���΁A������ꏊ�Ɠ_���v�Z�p�̒l���擾����
	if (chain_entry)
	{
		Score bonus = SCORE_ZERO;

		// �A�����N���Ȃ��Ȃ�܂�
		while (chain_entry->chainElement()->puyoNum())
		{
			// �G���g���[����A���v�f�𓾂�B
			ce.set(chain_entry->chainElement());

			// �{�[�i�X�_���v�Z
			bonus += ce.chainScore();
			
			if (!bonus)
				bonus = Score(1);

			// x�A���ڂ͒��ׂ��̂ŁAx + 1�A���ڂɂ���
			ce.plusChain();

			// ���̋ǖʂɂ��Ē��ׂ���A���̋ǖʂ̃n�b�V���L�[�𓾂�probe����B
			chain_entry = CT.probe(chain_entry->afterKey());

			if (!chain_entry)
			{
				// �r���̃G���g���[���󂳂�Ă����B���̎��͎d�����Ȃ��̂ŔՖʂ�ω�������B
				entry_deleted = true;
				break;
			}
		}

		// ���ׂēo�^����Ă����D
		if (!entry_deleted)
		{
			// �Ăяo�����ŉ��A���������̂���chainMax()���\�b�h�ŋ��߂���悤�ɁB
			chain_max_ = ce.chain();

			if(bonus && flag(ALLCLEAR))
				bonus += RATE * Score(30);

			return score() + bonus;
		}
	}
	
	// 1�����
	if(deleteMin())
	{
		return deleteScoreDestroy();
	}

	return score();
}

// �󔒂�����Ȃ�X���C�h������Brecul_y_���v�Z���Ă�������
// true�ŌĂяo���΁C�n�b�V���̍����v�Z������D
template <bool Hash> void LightField::slideMin()
{
	for (int x = 1; x <= 6; x++)
	{
		if (recul(x) == RANK_MAX)
			continue;

		//for (Square sq = toSquare(x, recul(x) == RANK_MAX ? upper(x) : recul(x)); toY(sq) < upper(x); ++sq)
		for (Square sq = toSquare(x, recul(x)); toY(sq) < upper(x); ++sq)
		{
			if (isEmpty(sq))// �󔒂���������
			{
				// �ŏ���empty�ȏꏊ���o���Ă����Ȃ���΂Ȃ�Ȃ����߁A�R�s�[����B
				Square dsq = sq;

				// ���̏�����̂Ղ��T��
				do {
					++dsq;
				} while (isEmpty(dsq) && toY(dsq) < upper(x));

				// ������ɉ����Ȃ����
				if (toY(dsq) >= upper(x))
				{
					upper_y_[x] = toY(sq);
					break;
				}

				while (toY(dsq) < upper(x))
				{
					// �������Ղ���A�󔒂̏ꏊ�Ɉڂ�
					//std::swap(field_[dsq], field_[sq]);
					field_[sq] = field_[dsq];
					field_[dsq] = EMPTY;

					if (Hash)
					{
						hash_key_ ^= hash_->seed(dsq, color(sq));
						hash_key_ ^= hash_->seed(sq, color(sq));
					}
					++sq;

					do {
						++dsq;
					} while (isEmpty(dsq) && toY(dsq) < upper(x));
				}
				upper_y_[x] = toY(sq);// ��ԍŌ��swap�����ʒu���ŏ�ʂՂ�
				break;
			}
		}
	}
}

// bb��1�ɂȂ��Ă���Ƃ��������
// �Čv�Z�͈͂������ł��ׂČv�Z����̂ŁArecul_y_�̑O�����Ƃ�����Ȃ���
// Hash��true�ɂ�����n�b�V���̍����v�Z������
template <bool Hash> void LightField::deletePuyo(Bitboard& bb_delete)
{
	// �n�b�V���̍����v�Z�����Ȃ� == �u���\�ɃG���g���[�������� == bb_delete�͂�����܂Ղ�̂���ꏊ��1�ɂȂ��Ă���
	if (Hash && field_ojama_num_)
	{
		Bitboard bb = bb_delete;

		while (bb.isTrue())
		{
			const Square sq = bb.firstOne01();

			if (Hash)
				hash_key_ ^= hash_->seed(sq, color(sq));

			field_[sq] = EMPTY;

			for (Rotate r = ROTATE_UP; r < ROTATE_MAX; ++r)// 4�������ׂ�
			{
				const Square next_sq = sq + rotatePosition(r);

				if (field_[next_sq] == OJAMA)
				{
					if (Hash)
						hash_key_ ^= hash_->seed(next_sq, OJAMA);

					field_[next_sq] = EMPTY;
					bb_delete.set(next_sq);
				}
			}
		}
		// �E�����ƍ������ɕ����Ē��ׂ邱�Ƃō�������}��B
		for (int x = 1; x <= 3; x++)
		{
			Bitboard bb_line = bb_delete.line(x);
			recul_y_[x] = bb_line.b(0) ? toY(bb_line.leftOne()) : RANK_MAX;
		}
		for (int x = 4; x <= 6; x++)
		{
			Bitboard bb_line = bb_delete.line(x);
			recul_y_[x] = bb_line.b(1) ? toY(bb_line.rightOne()) : RANK_MAX;
		}
	}
	else
	{
		for (int x = 1; x <= 3; x++)
		{
			Bitboard bb_line = bb_delete.line(x);

			if (bb_line.b(0))
			{
				const Square sq = bb_line.leftOne();
				recul_y_[x] = toY(sq);

				if (Hash)
					hash_key_ ^= hash_->seed(sq, color(sq));

				field_[sq] = EMPTY;

				while (bb_line.b(0))
				{
					const Square sq2 = bb_line.leftOne();

					if (Hash)
						hash_key_ ^= hash_->seed(sq2, color(sq2));

					field_[sq2] = EMPTY;
				}
			}
			else
			{
				recul_y_[x] = RANK_MAX;
				continue;
			}
		}
		for (int x = 4; x <= 6; x++)
		{
			Bitboard bb_line = bb_delete.line(x);

			if (bb_line.b(1))
			{
				const Square sq = bb_line.rightOne();
				recul_y_[x] = toY(sq);

				if (Hash)
					hash_key_ ^= hash_->seed(sq, color(sq));

				field_[sq] = EMPTY;

				while (bb_line.b(1))
				{
					const Square sq2 = bb_line.rightOne();

					if (Hash)
						hash_key_ ^= hash_->seed(sq2, color(sq2));

					field_[sq2] = EMPTY;
				}
			}
			else
			{
				recul_y_[x] = RANK_MAX;
				continue;
			}
		}
	}
}

// field_[x][y] �̎���4������T���A�����F������΂Ȃ����Ă���ꏊ�̏���field�ɃZ�b�g����
// rotate�������ɂƂ�Ƃ��́A����rotate�����͒��ׂȂ�
void LightField::setConnect(const Square sq, int rotate)
{
	if (toY(sq) >= 13)
	{
		if (toY(sq) == 13)
			connect_[0]++;

		return;// 14�i�ڂ�connect���Ȃ�
	}

	assert(puyo(sq) != EMPTY && puyo(sq) != OJAMA);

	int con_dir_num = 0;

	// TODO: ROTATE_UP�𒲂ׂȂ���΂Ȃ�Ȃ��̂�tumo������ڂ̂Ƃ������B
	for (Rotate r = ROTATE_UP; r < ROTATE_MAX; ++r)
	{
		if (r == rotate || (toY(sq) == RANK_12 && r == ROTATE_UP))
			continue;

		const Square next_sq = sq + rotatePosition(r);

		if (color(sq) == color(next_sq))
		{
			con_dir_num++;
			field_[sq] |= toDir(r);
			field_[next_sq] |= toDirRev(r);
		}
	}

	if (flag(VANISH))
	{
		// �ǂ�����ōČv�Z���K�v�ɂȂ�̂ŁA���������ɖ߂�B
		// �R�l�N�g���̌v�Z�͂��Ȃ���΂Ȃ�Ȃ��i�A���̂Ƃ��ɗ��p���邽�߁j
		return;
	}

	// ������t���O�������Ă��Ȃ��ꍇ�́A�R�l�N�g��(!=�R�l�N�g���j�͂����ƌv�Z����
	else if (con_dir_num == 3)// ��x�ɂȂ�������͍ő��3�ӏ��B���̂Ƃ��͂S�Ȃ����Ă��邱�Ƃ��ۏႳ��Ă���
	{
		setFlag(VANISH);
	}
	else if (con_dir_num > 0)
	{
		// x,y���炢���Ȃ������̂��B
		int con_num = bbSameColor(sq).count();

		if (con_num >= 4) // 4�q�����Ă���
			setFlag(VANISH);
		else if (con_num == 3)// 3�q�����Ă���
		{
			connect_[2]++;// 3�q�����Ă���ꏊ�͂ЂƂ�����

			if (con_dir_num == 2)// �q�����Ă��������2�ӏ� 1�q�����Ă���Ƃ����2����	
				connect_[0] -= 2;
			else if (con_dir_num == 1)
				connect_[1]--;
			else
				assert(false);// �����ɂ����炾��
		}
		else if (con_num == 2)// 2�q�����Ă���
		{
			assert(con_dir_num == 1);
			connect_[0]--;
			connect_[1]++;
		}
	}
	else
		connect_[0]++;
}

// field_[x][y] �̎���4������T���A�����F������΂Ȃ����Ă���ꏊ�̏���field�ɃZ�b�g����
void LightField::setConnectMin(const Square sq)
{
	if (toY(sq) >= 13)
		return;

	assert(puyo(sq) != EMPTY && puyo(sq) != OJAMA);

	int con_dir_num = 0;

	for (Rotate r = ROTATE_UP; r < ROTATE_MAX; ++r)
	{
		const Square next_sq = sq + rotatePosition(r);

		if (color(sq) == color(next_sq))
		{
			con_dir_num++;
			field_[sq] |= toDir(r);
			field_[next_sq] |= toDirRev(r);
		}
	}

	if (flag(VANISH))
	{
		// �ǂ�����ōČv�Z���K�v�ɂȂ�̂ŁA���������ɖ߂�B
		// �R�l�N�g���̌v�Z�͂��Ȃ���΂Ȃ�Ȃ��i�A���̂Ƃ��ɗ��p���邽�߁j
		return;
	}
	else if (con_dir_num == 3)// ��x�ɂȂ�������͍ő��3�ӏ��B���̂Ƃ��͂S�Ȃ����Ă��邱�Ƃ��ۏႳ��Ă���
	{
		setFlag(VANISH);
	}
	else if (con_dir_num > 0)
	{
		int con_num = bbSameColor(sq).count();

		if (con_num >= 4) // 4�q�����Ă���
			setFlag(VANISH);
	}
}

bool LightField::operator==(const LightField &f) const
{
	if (hash_key_ != f.hash_key_)
		return false;

	for (int x = 1; x <= 6; x++)
	{
		if (upper(x) != f.upper(x))
		{
			return false;
		}

		for (int y = 0; y <= 14; y++)
		{
			const Square sq = toSquare(x, y);

			if (field_[sq] != f.field_[sq])
			{
				return false;
			}
		}
	}

	return true;
}

bool LightField::isEmpty()
{
	for (int x = 1; x <= 6; x++)
		if (field_[toSquare(x, 1)] != EMPTY)
			return false;

	return true;
}
bool LightField::isAllClear() const
{
	for (int x = 1; x <= 6; x++)
		if (upper(x) != 1)
			return false;

	return true;
}