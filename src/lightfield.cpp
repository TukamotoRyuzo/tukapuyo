#include "field.h"
#include "move.h"
#include "bitboard.h"
#include "chain.h"
#include "tt.h"

Color LightField::use_color_[4];

//#define DEBUG

// フィールドから最大の連鎖を見つけ，そのスコアを返す．厳しめのチェックと甘めのチェックがあり，この関数はそれを呼びわけるラッパーである．
// 戻り値はお邪魔ぷよ何個分という値には変換していない．
Score LightField::maxChainsScore(const int ply_max, const int remain_time)
{
	chain_called++;

	// 前回以前の連鎖の情報をクリアする。利用することもできるかもしれない。
	chainsList()->clear();
	chainsList()->push_back(Chains(0, 0, 0));

	const Key key = this->key();
	ChainsInfo* entry = CIT[key];
	Score score;

	if (entry->key == (key >> 32))
	{
		chain_hit++;

		*chainsList() = entry->cl;

		if (field_puyo_num_ > 42 || ((ojama_ >= 12 || ojama_ >= deadLine()) && (remain_time > 0 && remain_time <= ONEPUT_TIME * 3)))
		{
			// 2手以内に打てる最大の連鎖
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
		// 後 1, 2手しか積めないのなら、実際のツモだけで連鎖をシミュレートする
		if (field_puyo_num_ > 42 || ((ojama_ >= 12 || ojama_ >= deadLine()) && (remain_time > 0 && remain_time <= ONEPUT_TIME * 3)))
		{
			score = searchChains(3);

			if (score == SCORE_MATED)
				return SCORE_MATED;
		}
		else
		{
			// 2手以内に打てる連鎖のリストを構築する．
			const Score chains_score = searchChains(2);

			if (chains_score == SCORE_MATED)
				return SCORE_MATED;

			// 自分にとって致死量の連鎖を打たれて，かつ後3手ぐらいしか置けない場合は，最大限厳し目のチェックをする．
			if (ojama_ > deadLine() && remain_time < ONEPUT_TIME * 5)
			{
				// 2個以上つながっているぷよを消す．3手でほしい色が2つ引ける可能性は56%
				score = std::max(chains_score, chainBonus(2));
			}
			else
			{
				// 連鎖を打たれていない場合や4手以上余裕があるなら，色補完をして連鎖があるかどうか調べる．
				// 2列以上振る連鎖を相手に打たれていて時間もあまりないなら，色補完は一つだけで調べる．
				const int help_max = ojama_ > 12 ? 1 : 2;

				// ぷよぷよが8列以上積まれているなら，連鎖を見つける条件を厳しくする．
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

// searchChainsを呼び出すためのラッパーだが,意味はdebugするためと、見た目のため。
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

// ply_max手かけて起こせる連鎖のうち一番大きな連鎖の点数を返す。
// ply_max手だけ探索して連鎖が起きるかどうかをシミュレートする。
// 終盤においてはこれを呼び出すほうがよい．そうでない場合は違う関数．
// 帰ってくる値はお邪魔何個分とか言う値ではなく，生の点数．
// const 関数ではないが const 関数なはず。
// TODO: 打った後の形の良さ(positionBonus())も記憶しておくべきか？
Score LightField::searchChains(const int ply_max, int ply)
{
	static int taketime_stack[3] = { 0 };
	if (ply == ply_max)
	{
		// ここにくる場合は連鎖が起きなかった場合なので、負けか0を返す。
		return isDeath() ? SCORE_MATED : SCORE_ZERO;
	}

	Move mlist[22];
	int movenum = generateMoves(mlist);
	Score best_score = SCORE_MIN - Score(1);
	Score s;

	// 手を進め、ハッシュも差分計算する。
	nextPlus();

	// for debug
	const Key w_key = key();

	for (int i = 0; i < movenum; i++)
	{
		doMove<false>(mlist[i]);

		// 設置にかかる時間を計算
		// 落下にかかる時間は、13 - 設置位置のyの小さいほう ちぎりなら、余計に時間がかかる
		taketime_stack[ply] = (13 - std::min(toY(mlist[i].csq()), toY(mlist[i].psq()))) * FALLTIME + (mlist[i].isTigiri() ? TAKETIME_INCLUDETIGIRI : TAKETIME);

		if (flag(VANISH))
		{
			// 消えるなら点数を計算する。
			s = deleteScore();

			// お邪魔4個以上振るなら連鎖として認める．
			if (s > 4 * RATE)
			{
				int take_frame = 0;
				for (int y = 0; y <= ply; y++)
					take_frame += taketime_stack[y];
				// 連鎖で発生するお邪魔ぷよ数、連鎖にかかるフレーム数、発火にかかるフレーム
				Chains entry = Chains(s / RATE, CHAINTIME * chainMax(), take_frame);

				// もし登録済みの連鎖なら登録しない．
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

// 表面に露出しているぷよを総当りで消し，一番大きな連鎖のスコアを返す．
// connect_num 以上連結している場所しか消さない。
Score LightField::chainBonus(const int connect_num) const 
{
	assert(recul(0) == RANK_MAX && recul(7) == RANK_MAX);

	int left, right;
	setRange2(&left, &right);

	Score max = SCORE_ZERO, s = SCORE_ZERO;

	// 露出しているぷよを消す
	for (int x = left; x <= right; x++)
	{
		assert(x >= 1 && x <= 6);
		assert(isInSquare(x, upper(x) - 1));

		// おじゃまぷよを消すことも許す。
		for (int y = upper(x) - 1; y >= 1; y--)
		{
			const Square sq = toSquare(x, y);

			// 露出している方向を調べる
			Connect c = CON_NONE;

			if (isEmpty(sq + SQ_UP))
				c |= CON_UP;
			if (isEmpty(sq + SQ_RIGHT))
				c |= CON_RIGHT;
			if (isEmpty(sq + SQ_LEFT))
				c |= CON_LEFT;

			// 露出していない == それより下のぷよも露出していない
			if (c == CON_NONE)
				break;

			const Color cl = color(sq);

			// 2回以上同じ消し方をしないために履歴を取っておく
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

				// 最初はiが0なのでこのforには入らない。
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

					// スライドは発生しない
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

// フィールドの置ける場所に色を補完して連鎖があるかどうか調べる。
// 補完する個数の最大がhelp_max
// debug : const 関数になっていなければassertする。
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

// フィールドの置ける場所に色を補完して連鎖があるかどうか調べる。
// 補完する個数の最大がhelp_max　const 関数ではないが const 関数なはず。
Score LightField::colorHelper(const int help_max, int help_num)
{
	// 残り補完数
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
			const Key key = this->key() + remain_help;

			// 局面表を見て探索済みなら値を得てcontinue.
			EvaluateHashEntry entry = *ET[key];

			if (entry.key() == (key >> 32) && entry.remainHelp() == remain_help)
			{
				++allready_searched;
				s = entry.score();
			}
			else
			{
				// ここが最大深さである
				if (help_num >= help_max)
				{	
					s = chainBonus(1);
				}
				else
				{
					s = colorHelper(help_max, help_num + 1);
				}
				
				// エントリーに登録
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

int LightField::examine() const // Fieldの整合性をチェックする
{
	Key hash_key_w = 0;
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

			if (connect(sq))// どこかのぷよとつながっている場合
			{
				if (color(sq) == EMPTY || color(sq) == OJAMA || color(sq) == WALL)
					assert(false);

				for (Connect c = connect(sq); c; c &= c - 1)
				{
					// つながっているぷよ側から見て、自分の方向につながっていない
					// または、自分と同じ色ではない
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
					// 何かぷよが置かれていたら
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
					// 下方向にEMTPYがあったらだめ
					for (int d = 1; y - d >= 1; d++)
					{
						if (isEmpty(toSquare(x, y - d)))
							assert(false);
					}

					// ↑方向にEMPTYではないところがあったらだめ
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

// AI用に定義
LightField::LightField(const Field &f)
{
	// fieldから必要なメンバだけコピー
	memcpy(this, &f, sizeof(LightField));
}


// フィールド上に同じ色を探す。
// 同じ色になったところは1にする。
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

// bitboard をセットするだけ
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

// sqから同じ色でつながっている場所が1になっているbitboardを返す。
Bitboard LightField::bbSameColor(const Square sq) const
{
	Bitboard bb(0, 0);
	setSameColor(bb, sq);
	return bb;
}

// field_[x][y]の周りにつながっているぷよがあった場合、
// field_[x][y]を消す際、周りのコネクト情報が残ってしまうため,まわりのコネクトを元に戻す
void LightField::resetConnect(const Square sq)
{
	assert(color(sq) != EMPTY && color(sq) != OJAMA);

	// 13段目はconnectしない
	if (toY(sq) >= 13)
		return;

	for (Connect c = connect(sq); c; c &= c - 1)
	{
		// 方向
		const Rotate r = lsbToRotate(c);

		field_[sq + rotatePosition(r)] &= ~toDirRev(r);
	}
}

// 合法手生成
int LightField::generateMoves(Move* move) const
{
	const Move* begin = move;

	// 3,12が詰まっているときは死んでいる局面
	if (upper(3) >= RANK_13)
		return 0;

	int right, left;
	setRange(&left, &right);

	const Tumo now = getNowTumo();
	const bool color_is_diff = (now.pColor() != now.cColor());

	for (int x = right; x >= left; x--)
	{
		const int up = upper(x);
		const int up_side = upper(x + 1);
		const Square dest = toSquare(x, up);
		const Square dest_up = dest + SQ_UP;

		assert(isEmpty(dest) && !isEmpty(dest + SQ_DOWN));

		// 回転数0でおけるなら2でも置けるし、1でおけるなら3でも置ける
		if (color_is_diff)
		{
			if (up_side < RANK_13)
			{
				const Square dest_side = toSquare(x + 1, up_side);
				const bool is_tigiri = (up != up_side);

				// 4手一気に生成。
				*(uint64_t*)move = (uint64_t)dest_up << 56 | (uint64_t)dest << 48				   // rotate == 0
					| (uint64_t)dest << 40 | (uint64_t)dest_up << 32							   // rotate == 2
					| (uint64_t)is_tigiri << 31 | (uint64_t)dest_side << 24 | (uint64_t)dest << 16 // rotate == 1
					| (uint64_t)is_tigiri << 15 | (uint64_t)dest << 8 | (uint64_t)dest_side << 0;  // rotate == 3
				move += 4;
			}
			else
			{
				// 2手ずつ生成。
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

				// 2手ずつ生成。
				*(uint32_t*)move = dest_up << 24 | dest << 16
					| is_tigiri << 15 | dest_side << 8 | dest << 0;
				move += 2;
			}
			else
			{
				// 1手だけ生成。
				*(move++) = Move(dest, dest_up, false);
			}
		}

	}

	// 生成した手の数
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

// moveした後、連鎖、おじゃまぷよの落下があるなら落下を行う
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

	// 差分計算できるのでする
	hash_key_ ^= hash_->seed(psq, pc);
	hash_key_ ^= hash_->seed(csq, cc);

	field_puyo_num_ += 2;

	// 手にかかる時間を計算し、自分の残り時間から引く
	int fall_time = (12 - std::min(toY(csq), toY(psq)));

	// 落下ボーナス
	score_ += fall_time;

	// もし11段目より上に置く手なら、x方向の移動も時間に入れる。
	if (fall_time <= FALLTIME)
		fall_time += abs(3 - toX(psq));

	assert(!(ojama_ == 0 && flag(OJAMA_WILLFALL)));

	const bool vanish = flag(VANISH);

	if (vanish)
	{
		clearFlag(VANISH);

		// 連鎖が終わるまで消す。このメソッドで消した後のコネクト情報は再計算されている
		while (deleteMin());

		if (isAllClear())
		{
			setFlag(ALLCLEAR);
			hash_key_ ^= hash_->seed();
		}

		// コネクト数の再計算
		if (bonusInit() == -1)
			assert(false);

		// 相殺
		offseting(enemy);

		// もし相手に返しきったら、次に相手におじゃまぷよが降るフラグを立てておく
		if (enemy.ojama_)
			enemy.setFlag(OJAMA_WILLFALL);
	}

	int put_time = fall_time * FALLTIME									// 落下時間
		+ (move.isTigiri() ? TAKETIME_INCLUDETIGIRI : TAKETIME)			// 硬直時間。ちぎりなら、追加時間
		+ (vanish ? CHAINTIME * chainMax() + NEXTTIME : 0);				// 連鎖なら、連鎖時間 + 硬直時間

	chain_max_ = 0;

	// ここで、自分にお邪魔ぷよが降るなら降らせる
	// お邪魔ぷよが振る時間と、相手が行動可能になるまでの時間は少し違う。
	// 連鎖後の硬直時間を引いた値がお邪魔ぷよが振るまでのタイムリミット
	if (flag(OJAMA_WILLFALL) && my_remain_time - put_time - TAKETIME - 1 <= 0)
	{
		assert(ojama_ != 0);

		ojamaFallMin(&ojama_);

		if (ojama_ == 0)
			clearFlag(OJAMA_WILLFALL);

		// お邪魔ぷよが振ったときは、更に硬直時間がある。
		put_time += NEXTTIME;
	}

	assert(examine());
	return put_time;
}

// 局面を戻す
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

// ボーナスの計算をしない
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

// お邪魔ぷよが降るとき、これを呼び出す
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

		// シャッフル
		for (int n = 0; n < 6; n++)
		{
			int r = ojama_pool_[ojama_rand_id_];
			ojama_rand_id_ = (ojama_rand_id_ + 1) & 127;
			std::swap(rand_ojama[n], rand_ojama[r]);
		}
	}

	// おじゃまぷよが6個以下になるまでは，下から順番にお邪魔ぷよを並べていく
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

	// 6個以下になったら，ランダムに置く．
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

// 相殺する
void LightField::offseting(LightField& enemy)
{
	int send_ojama_num = score_ / RATE;

	if (send_ojama_num)
	{
		if (ojama_)// もし自分にお邪魔がふりそうなら
		{
			ojama_ -= send_ojama_num;// 相殺

			if (ojama_ < 0)
			{
				enemy.ojama_ += -ojama_;// マイナスになった分に関しては、相手にあげる
				ojama_ = 0;
				clearFlag(OJAMA_WILLFALL);
			}
			else if (ojama_ == 0)
				clearFlag(OJAMA_WILLFALL);
		}
		else
		{
			enemy.ojama_ += send_ojama_num;// おじゃまぷよをおくる
			assert(!flag(OJAMA_WILLFALL));
		}

		score_ %= RATE;
	}
}

// 局面のハッシュ値のセット。
// 差分計算はせず、ここで一気にやる。
Key LightField::keyInit()
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


// 連鎖後、コネクト数の再計算を行う
// ここでツモ数も数えておく
// ついでにボーナス点の計算も行う
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
				// 何かぷよが置かれていたら
				int count = findSameColor(bb_same, sq);
				
				assert(count < 4);
				connect_[count - 1]++;
			}
		}
	}
	assert(connect_[0] + connect_[1] * 2 + connect_[2] * 3 == field_puyo_num_);
	return 0;
}

// 再計算が必要な範囲だけ、connect情報をclearする
// もし連結を断つ処理が発生したならtrueを返す。もしfalseならスライドは発生しないことがわかる。
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
			field_[sq + SQ_RIGHT] &= ~CON_LEFT;// 右方向から見れば、左につながっているので
			field_[sq + SQ_LEFT] &= ~CON_RIGHT;// 左方向から見れば、右につながっているので
			field_[sq] &= 0xf0;
		}
	}

	return connect_clear;
}

// 再計算が必要な範囲だけ、connect情報をsetする
void LightField::setConnectRecul()
{
	for (int x = 1; x <= 6; x++)
	{
		for (int y = recul(x); y < upper(x); y++)// 13段目のぷよはコネクトしていない
		{
			assert(y >= 0 && y <= 13);
			const Square sq = toSquare(x, y);

			if (y < RANK_13 && !isEmpty(sq) && puyo(sq) != OJAMA)
			{
				// 上方向を調べる必要はない。なぜなら一つ上のぷよの下方向を調べてつながっていれば
				// 上につながっているはずだから。
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

// chain_ = 連鎖数
// score_ = この連鎖の点数
// コネクト情報の再計算を行う。コネクト数の再計算はしない。
// 戻り値：連鎖が起こらなくなったらfalse
bool LightField::deleteMin()
{
	// この局面のキー
	const Key key = this->key();

	// 連鎖テーブルから既に調べた形ではないかを調べる。
	const ChainEntry* chain_entry = CT.probe(key);

	ChainElement ce(chain());
	Bitboard bb_over4;

	// 消えるならtrueになる
	bool can_delete;

	// 登録されている形であれば、消える場所と点数計算用の値を取得する
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
			// 連鎖は最低でも1点以上はあるので、0点なら連鎖が起きない形であるということ。
			can_delete = false;
		}
	}
	else // そうでなければ、フィールドを走査して調べる。
	{
		Bitboard bb_allready(0, 0);
		int color_bit = 0;
		bb_over4 = Bitboard(0, 0);

		if (chain_ == 0)
		{
			// 最適化されるはず
			for (int i = 1; i <= 6; i++)
				recul_y_[i] = RANK_2;
		}

		for (int x = 1; x <= 6; x++)
		{
			// 今調べているx列目と、左隣と右隣の最小値番目の段から調べる
			// int y = 1と書くよりはずっと早い
			const int wy = min3(recul(x - 1), recul(x), recul(x + 1));
			assert(wy > 0);

			for (int y = wy - 1; y < upper(x); y++)
			{
				const Square sq = toSquare(x, y);
				const Connect c = connect(sq);

				// つながっているぷよが2個以下
				// 連鎖がおこるときは、最低でもひとつは二つ以上つながっているぷよが存在する
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

		// 連鎖が起こるなら、最低でもひとつはcolor_bitが立つ。
		if (color_bit)
		{
			can_delete = true;
			ce.setColorBonus(Bitop::popCount(color_bit));
			ce.setPuyoNum(bb_over4.count());
		}
		else
			can_delete = false;
	}

	if (can_delete)// 連鎖が起きた場合
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

		if (flag(ALLCLEAR))// 全消しなら
		{
			score_ += RATE * 30;
			clearFlag(ALLCLEAR);
			hash_key_ ^= hash_->seed();
		}

		chain_++;
		score_ += bonus;

		// connect情報の再計算
		clearConnect();

		if (chain_entry)
		{
			// スライド. ハッシュの差分計算はなし
			slideMin<false>();
		}
		else
		{
			// ハッシュの差分計算もする
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
			// 次に調べるときにキー値が正しくなければいけない。
			assert(this->key() == this->keyInit());

			// この局面を登録しておく。
			CT.store(key, this->key(), ce, bb_over4);
		}
	}
	else
	{
		// この局面を登録しておく。
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

// 連鎖の点数だけを知りたいときは、既に調べたことのある局面ならこちらが高速
// 連鎖の点数だけ知りたいときというのは、評価関数内で自分が持っている連鎖リストを構築するときに
// その連鎖の点数だけわかればよく、実際に局面を進める必要がない場合を指す。
// メンバ変数chain_max_を変化させるのでconst関数ではない。しかしそれ以外はconst関数である。
Score LightField::deleteScore()
{
	chain_called++;

	// この局面のキー
	const Key key = this->key();

	// 連鎖テーブルから既に調べた形ではないかを調べる。
	const ChainEntry* chain_entry = CT.probe(key);

	ChainElement ce(chain());

	// 途中のエントリーが壊れているか．
	bool entry_deleted = false;

	// 登録されている形であれば、消える場所と点数計算用の値を取得する
	if (chain_entry)
	{
		chain_hit++;

		Score bonus = SCORE_ZERO;

		// 連鎖が起きなくなるまで
		while (chain_entry->chainElement()->puyoNum())
		{
			// エントリーから連鎖要素を得る。
			ce.set(chain_entry->chainElement());

			// ボーナス点を計算
			bonus += ce.chainScore();
			
			if (!bonus)
				bonus = Score(1);

			// x連鎖目は調べたので、x + 1連鎖目にする
			ce.plusChain();

			// この局面について調べたら、次の局面のハッシュキーを得てprobeする。
			chain_entry = CT.probe(chain_entry->afterKey());

			if (!chain_entry)
			{			
				// 途中のエントリーが壊されていた。この時は仕方がないので盤面を変化させる。
				entry_deleted = true;
				break;
			}
		}

		if (!entry_deleted)
		{
			// 呼び出し元で何連鎖だったのかをchainMax()メソッドで求められるように。
			chain_max_ = ce.chain();

			if(bonus && flag(ALLCLEAR))
				bonus += RATE * Score(30);

			return score() + bonus;
		}
	}
	
	// 連鎖テーブルに登録されていないか、登録されていても途中のエントリーが壊れていた場合はここにくる。
	LightField f(*this);

	Score s = SCORE_ZERO;

	// 1回消す
	if(f.deleteMin())
	{
		s = f.deleteScoreDestroy();
	}

	// 呼び出し元で何連鎖だったのかをchainMax()メソッドで求められるように。
	chain_max_ = f.chainMax();
	return f.score();
}

// 局面がぶっ壊れてもいいのでとにかく連鎖スコアがほしいときはこれのほうが早い．
// 局面のコピーを作って呼び出すときにはこれを使うといい．
Score LightField::deleteScoreDestroy()
{
	// この局面のキー
	const Key key = this->key();

	// 連鎖テーブルから既に調べた形ではないかを調べる。
	const ChainEntry* chain_entry = CT.probe(key);

	ChainElement ce(chain());

	// 途中のエントリーが壊れているか．
	bool entry_deleted = false;

	// 登録されている形であれば、消える場所と点数計算用の値を取得する
	if (chain_entry)
	{
		Score bonus = SCORE_ZERO;

		// 連鎖が起きなくなるまで
		while (chain_entry->chainElement()->puyoNum())
		{
			// エントリーから連鎖要素を得る。
			ce.set(chain_entry->chainElement());

			// ボーナス点を計算
			bonus += ce.chainScore();
			
			if (!bonus)
				bonus = Score(1);

			// x連鎖目は調べたので、x + 1連鎖目にする
			ce.plusChain();

			// この局面について調べたら、次の局面のハッシュキーを得てprobeする。
			chain_entry = CT.probe(chain_entry->afterKey());

			if (!chain_entry)
			{
				// 途中のエントリーが壊されていた。この時は仕方がないので盤面を変化させる。
				entry_deleted = true;
				break;
			}
		}

		// すべて登録されていた．
		if (!entry_deleted)
		{
			// 呼び出し元で何連鎖だったのかをchainMax()メソッドで求められるように。
			chain_max_ = ce.chain();

			if(bonus && flag(ALLCLEAR))
				bonus += RATE * Score(30);

			return score() + bonus;
		}
	}
	
	// 1回消す
	if(deleteMin())
	{
		return deleteScoreDestroy();
	}

	return score();
}

// 空白があるならスライドさせる。recul_y_を計算しておくこと
// trueで呼び出せば，ハッシュの差分計算をする．
template <bool Hash> void LightField::slideMin()
{
	for (int x = 1; x <= 6; x++)
	{
		if (recul(x) == RANK_MAX)
			continue;

		//for (Square sq = toSquare(x, recul(x) == RANK_MAX ? upper(x) : recul(x)); toY(sq) < upper(x); ++sq)
		for (Square sq = toSquare(x, recul(x)); toY(sq) < upper(x); ++sq)
		{
			if (isEmpty(sq))// 空白を見つけたら
			{
				// 最初のemptyな場所を覚えておかなければならないため、コピーする。
				Square dsq = sq;

				// その上方向のぷよを探す
				do {
					++dsq;
				} while (isEmpty(dsq) && toY(dsq) < upper(x));

				// 上方向に何もなければ
				if (toY(dsq) >= upper(x))
				{
					upper_y_[x] = toY(sq);
					break;
				}

				while (toY(dsq) < upper(x))
				{
					// 見つけたぷよを、空白の場所に移す
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
				upper_y_[x] = toY(sq);// 一番最後にswapした位置が最上位ぷよ
				break;
			}
		}
	}
}

// bbが1になっているところを消す
// 再計算範囲もここですべて計算するので、recul_y_の前準備とかいらないよ
// Hashをtrueにしたらハッシュの差分計算をする
template <bool Hash> void LightField::deletePuyo(Bitboard& bb_delete)
{
	// ハッシュの差分計算をしない == 置換表にエントリーがあった == bb_deleteはおじゃまぷよのある場所も1になっている
	if (Hash && field_ojama_num_)
	{
		Bitboard bb = bb_delete;

		while (bb.isTrue())
		{
			const Square sq = bb.firstOne01();

			if (Hash)
				hash_key_ ^= hash_->seed(sq, color(sq));

			field_[sq] = EMPTY;

			for (Rotate r = ROTATE_UP; r < ROTATE_MAX; ++r)// 4方向調べる
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
		// 右半分と左半分に分けて調べることで高速化を図る。
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

// field_[x][y] の周り4方向を探し、同じ色があればつながっている場所の情報をfieldにセットする
// rotateを引数にとるときは、そのrotateだけは調べない
void LightField::setConnect(const Square sq, int rotate)
{
	if (toY(sq) >= 13)
	{
		if (toY(sq) == 13)
			connect_[0]++;

		return;// 14段目はconnectしない
	}

	assert(puyo(sq) != EMPTY && puyo(sq) != OJAMA);

	int con_dir_num = 0;

	// TODO: ROTATE_UPを調べなければならないのはtumoがぞろ目のときだけ。
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
		// どうせ後で再計算が必要になるので、何もせずに戻る。
		// コネクト情報の計算はしなければならない（連鎖のときに利用するため）
		return;
	}

	// 消えるフラグがたっていない場合は、コネクト数(!=コネクト情報）はちゃんと計算する
	else if (con_dir_num == 3)// 一度につながる方向は最大で3箇所。そのときは４つつながっていることが保障されている
	{
		setFlag(VANISH);
	}
	else if (con_dir_num > 0)
	{
		// x,yからいくつつながったのか。
		int con_num = bbSameColor(sq).count();

		if (con_num >= 4) // 4つ繋がっている
			setFlag(VANISH);
		else if (con_num == 3)// 3つ繋がっている
		{
			connect_[2]++;// 3つ繋がっている場所はひとつ増える

			if (con_dir_num == 2)// 繋がっている方向は2箇所 1つ繋がっているところは2個引く	
				connect_[0] -= 2;
			else if (con_dir_num == 1)
				connect_[1]--;
			else
				assert(false);// ここにきたらだめ
		}
		else if (con_num == 2)// 2つ繋がっている
		{
			assert(con_dir_num == 1);
			connect_[0]--;
			connect_[1]++;
		}
	}
	else
		connect_[0]++;
}

// field_[x][y] の周り4方向を探し、同じ色があればつながっている場所の情報をfieldにセットする
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
		// どうせ後で再計算が必要になるので、何もせずに戻る。
		// コネクト情報の計算はしなければならない（連鎖のときに利用するため）
		return;
	}
	else if (con_dir_num == 3)// 一度につながる方向は最大で3箇所。そのときは４つつながっていることが保障されている
	{
		setFlag(VANISH);
	}
	else if (con_dir_num > 0)
	{
		int con_num = bbSameColor(sq).count();

		if (con_num >= 4) // 4つ繋がっている
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