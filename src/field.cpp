#include <iostream>
#include "field.h"
#include "debug.h"
#include "bitboard.h"
#include "chain.h"

// ハッシュの種
namespace { const Hashseed hash1, hash2; }

// field_上にpuyoをセットする	
void Field::set(const Tumo &t)
{
	field_[t.psq()] = t.pColor();
	field_[t.csq()] = t.cColor();
}

// currentにEMPTYをセットする
void Field::setEmpty(const Tumo &t) { field_[t.psq()] = field_[t.csq()] = EMPTY; }

// 設置後、新しいツモをプールからもらう。
void Field::tumoReload()
{
	current_ = tumo_pool_[next_++];

	if (next_ >= 128)
		next_ = 0;
}

// 初期化
void Field::init()
{
	// 0クリアしたくないメンバだけ退避
	Flag s_flag = flag_;
	GameAssets* s_assets = assets;
	const Tumo* s_puyo_pool = tumo_pool_;
	const uint8_t* s_ojama_pool = ojama_pool_;

	// メンバをすべて0クリア
	memset(this, 0, sizeof(Field));

	// 元に戻す．
	flag_ = s_flag;
	assets = s_assets;
	tumo_pool_ = s_puyo_pool;
	ojama_pool_ = s_ojama_pool;

	for (int x = 0; x < FILE_MAX; x++)
	{
		if (x == 0 || x == 7)
			upper_y_[x] = RANK_15;
		else
			upper_y_[x] = RANK_1;
		
		for (int y = 0; y < FH; y++)
		{
			if (x == 0 || x == 7 || y == 0 || y == 15)
				field_[x * 16 + y] = WALL;
		}
		recul_y_[x] = RANK_MAX;
	}

	if (flag(PLAYER1))
		hash_ = &hash1;
	else
		hash_ = &hash2;

	// フラグのお掃除(PLAYERの情報以外を0にする)
	flag_ &= PLAYER_INFO;

	// ゲーム開始前はこの状態にする　
	setFlag(WAIT_OJAMA);
}

// 位置を渡し、移動させる。
// 成功すればtrue,失敗すればfalse
// 失敗した場合は何もしない
bool Field::putTumo(const Tumo &t)
{
	setEmpty(current());// 現在の場所を0にする

	// ぷよがおけない
	if (!isEmpty(t.psq()) || !isEmpty(t.csq()))
	{
		set(current());// 元に戻す
		return false;
	}
	else // 置けるので置く。
		set(t);

	return true;
}

// 一定時間ごとに一つ降りてくる処理
// その際、ちぎり発生 flag_ | TIGIRI
// そうでなく、位置確定時 flag_ | SET
void Field::dropTumo()
{
	Tumo t = current();

	t.down();// 一つ位置を下へ

	if (!putTumo(t))// もし置けないなら、そこで位置を確定させる
	{
		// ちぎり処理
		if (isEmpty(t.psq())) // もし位置確定しても、下に空きがあったら
		{
			const bool pivot_empty = isEmpty(t.psq());
			setFlag(TIGIRI);
			tigiri_ = current().psq();
			remain_ = current().csq();
			++upper_y_[current().cx()]; // 一番上の座標を覚えておく
			setConnect(current().csq(), current().rotateRev());
		}
		else if(isEmpty(t.csq()))
		{
			setFlag(TIGIRI);
			tigiri_ = current().csq();
			remain_ = current().psq();
			++upper_y_[current().px()]; 
			setConnect(current().psq(), current().rotate());
		}
		else
		{
			// ちぎりなし
			++upper_y_[current().px()];
			++upper_y_[current().cx()];

			// 子ぷよの方向は調べないことに注意
			// 連結数を調べているので、子ぷよの方向を連結させてしまうと連結数がおかしくなる
			setConnect(current().psq(), current().rotate());
			setConnect(current().csq());

			setFlag(SET);
		}
	}
	else // 設置未完了なので、位置を一つ下げる。
		current_ = t;
}

// 表示部
void Field::show()
{
	for (int i = 0; i < 2; i++)
	{
		ColorType c = colorToColorType(tumo_pool_[next_].color(1 - i));
		ColorType nc = colorToColorType(tumo_pool_[(next_ + 1) < 128 ? next_ + 1 : 0].color(1 - i));
		const int nextx = flag(PLAYER1) ? P1_N_BEGINX : P2_N_BEGINX;
		const int nexty = flag(PLAYER1) ? P1_N_BEGINY : P2_N_BEGINY;
		const int next2x = flag(PLAYER1) ? P1_NN_BEGINX : P2_NN_BEGINX;
		const int next2y = flag(PLAYER1) ? P1_NN_BEGINY : P2_NN_BEGINY;
		const int srcx = flag(PLAYER1) ? 0 : 17;

		// NEXTぷよを表示
		DxLib::DrawRectGraph(nextx, nexty + P_SIZE * i, 0, P_SIZE * c, P_SIZE, P_SIZE, assets->tumo.handle(), TRUE, FALSE);

		// NEXTNEXTぷよを表示
		DxLib::DrawRectGraph(next2x, next2y + P_SIZE * i, srcx, P_SIZE * nc, P_SIZE - 17, P_SIZE, assets->tumo.handle(), TRUE, FALSE);
	}

	// "全消し"と表示
	if (flag(ALLCLEAR))
	{
		const int destx = flag(PLAYER1) ? 68 : 418;
		assets->zenkesi.setPosition(destx, 108);
		assets->zenkesi.draw();
	}

	// フィールド上のぷよを表示
	// 操作中のぷよは別に表示する
	for (int x = 1; x <= F_WID; x++)// 番兵を引く
	{
		for (int y = 1; y < F_HEI; y++)// 番兵、見えない部分を引く
		{
			const int destx = (flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + P_SIZE * (x - 1);
			const int desty = (flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + P_SIZE * (12 - y);
			const Square sq = toSquare(x, y);

			// おじゃまぷよ画像は、画像ファイル上でかなり離れた位置に存在しているので
			// ojamapuyoというGraph型のインスタンスとして持っている
			if (color(sq) == OJAMA)
			{
				assets->ojamapuyo.setPosition(destx, desty);
				assets->ojamapuyo.draw();
				continue;
			}

			// 連鎖中ではなく、かつ
			// 何もないか、操作中のぷよなら表示しない
			if (!flag(RENSA))
			{
				if (isEmpty(sq) || (sq == current().psq() || sq == current().csq() || sq == tigiri_))
					continue;
			}
			else
			{
				if (isEmpty(sq) && !(delete_mark_[sq] & VANISH))
					continue;
			}

			const ColorType ct = colorToColorType(color(sq));

			// ぷよが消える直前には、驚いた顔の画像を表示する
			if (delete_mark_[sq] & VANISH)
			{
				if (flag(WAIT_RENSA) && isEmpty(sq))
				{
					if ((chain_stage_ / 3) < 9)
					{
						const ColorType delete_ct = colorToColorType(static_cast<Color>(delete_mark_[sq] & 0xf0));

						if (delete_ct < TOJAMA)
						{
							assets->vanish_chip[delete_ct][chain_stage_ / 3].setPosition(destx - 90, desty - 78);
							assets->vanish_chip[delete_ct][chain_stage_ / 3].draw();
						}
					}
				}
				else
				{
					assets->vanish_face[ct].setPosition(destx, desty);
					assets->vanish_face[ct].draw();
				}
				continue;
			}

			assert(ct >= TRED && ct <= TOJAMA);

			const int srcx = P_SIZE * con_color[connect(sq)];
			const int srcy = P_SIZE * ct;

			// 現在のフィールドの状態を描画する
			DxLib::DrawRectGraph(destx, desty, srcx, srcy, P_SIZE, P_SIZE, assets->tumo.handle(), TRUE, FALSE);
		}
	}

	if (!flag(RENSA))
	{
		static const double extratex[SETTIME] = { 1.05, 1.1, 1.15, 1.2, 1.25, 1.3, 1.35, 1.4, 1.35, 1.3, 1.25, 1.2, 1.15, 1.1, 1.05 };
		static const double extratey[SETTIME] = { 0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95 };
		int y_offset = static_cast<int>(drop_offset_ + freedrop_offset_);
		int x_offset = static_cast<int>(move_offset_);

		move_offset_ = 0;

		// 現在操作中のぷよを表示
		Square pos[2];
		int px = current().px(), py = current().py(), cx = current().cx(), cy = current().cy();
		Square psq = current().psq(), csq = current().csq();

		// currentにぷよがなければ千切れてしまったものと考える
		if (isEmpty(psq))
			psq = tigiri_;
		else if (isEmpty(csq))
			csq = tigiri_;

		// y軸が下のほうのぷよをpos[0]に、上のほうのぷよをpos[1]に格納
		pos[0] = py < cy ? psq : csq;
		pos[1] = py < cy ? csq : psq;

		int dest_x[2];
		int dest_y[2];
		dest_x[0] = (flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + P_SIZE * (toX(pos[0]) - 1);
		dest_y[0] = (flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + P_SIZE * (12 - toY(pos[0]));
		dest_x[1] = (flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + P_SIZE * (toX(pos[1]) - 1);
		dest_y[1] = (flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + P_SIZE * (12 - toY(pos[1]));

		// dropTumo()が呼ばれたときの処理
		// ちぎりのオフセットは後で計算する
		// 連鎖中はとりあえずオフセットなし
		// 回転数が縦なら小さいほうの座標の下に何もない
		// 回転数が横ならそれぞれのぷよの下にぷよがない
		if (!flag(TIGIRI) && !flag(RENSA) &&
			((!current().isSide() && isEmpty(pos[0] + SQ_DOWN)) ||
			(  current().isSide() && isEmpty(psq + SQ_DOWN) && isEmpty(csq + SQ_DOWN))))
		{
			// 少しずつ下にずらして表示するためのオフセットを足す
			dest_y[0] += y_offset;
			dest_y[1] += y_offset;
		}

		// 表示する
		for (int i = 0; i < 2; i++)
		{
			// 14段目は表示しない
			// 13段目は表示する可能性はある                                                                                   
			if (toY(pos[i]) == RANK_14)
				continue;

			const Color c = color(pos[i]);

			// WAIT_NEXT時はcurrentにぷよが存在していない
			// またお邪魔が振ったとき、currentがお邪魔になることもある
			if (c < RED || c == OJAMA)
				continue;

			const ColorType ct = colorToColorType(c);
			int srcx = P_SIZE * con_color[connect(pos[i])];
			int srcy = P_SIZE * ct;

			// これは横移動のためのオフセット
			dest_x[i] += x_offset;

			if (pos[i] == tigiri_)
				dest_y[i] += y_offset;

			// 13段目にあるなら、下に見える部分を少しずつずらして表示
			if (toY(pos[i]) == RANK_13)
			{
				if ( !current().isSide() && isEmpty(pos[0] + SQ_DOWN) ||
					( current().isSide() && isEmpty(pos[0] + SQ_DOWN) && isEmpty(pos[1] + SQ_DOWN)))
				{
					DxLib::DrawRectGraph(
						dest_x[i],
						dest_y[i] + (P_SIZE - y_offset),
						srcx,
						srcy + (P_SIZE - y_offset),
						P_SIZE,
						y_offset,
						assets->tumo.handle(), TRUE, FALSE);
				}
			}
			// ぷよんと弾む表示
			// 連鎖中ではない
			// ちぎりが発生していない
			// ちぎりが発生していてちぎり硬直中であり、表示するぷよがちぎれて残ったほうのぷよである
			// ちぎり発生後の設置硬直であり、ちぎれたほうのぷよである
			else if (
				!flag(RENSA) &&
				flag(WAIT_TIGIRISET | WAIT_SET) &&
				(tigiri_ == SQ_ZERO || // 値が設定されていないときはSQ_ZEROになる仕様
				(flag(WAIT_TIGIRISET) && (pos[i] == remain_)) ||
				(flag(WAIT_SET) && (pos[i] == tigiri_))
				))
			{
				int stage = puyon_stage_;

				if (stage < SETTIME / 2)
					dest_y[i] += (stage / 2);
				else
					dest_y[i] += (SETTIME - stage) / 2;

				if (puyon_stage_ >= SETTIME)
					stage = SETTIME - 1;

				int handle = assets->normal_tumo[ct].handle();
				DrawRotaGraph3(
					dest_x[i] + 16,
					dest_y[i] + 16,
					16,
					16,
					extratex[stage],
					extratey[stage],
					0, handle, true);
			}
			else
			{
				if (!(delete_mark_[pos[i]] & VANISH))
				{
					if(pos[i] == current().csq())
					{
						DxLib::DrawRectGraph(dest_x[i] + (int)rotate_offset_x_, dest_y[i] + (int)rotate_offset_y_, srcx, srcy, P_SIZE, P_SIZE, assets->tumo.handle(), TRUE, FALSE);
						rotate_offset_x_ = 0;
						rotate_offset_y_ = 0;
					}
					else
						DxLib::DrawRectGraph(dest_x[i], dest_y[i], srcx, srcy, P_SIZE, P_SIZE, assets->tumo.handle(), TRUE, FALSE);
				}
			}
		}
	}

	// スコア表示
	// 描画先のx座標はスコアの桁数に依存
	// 桁数を求める
	int dstx, dsty;
	dsty = flag(PLAYER1) ? 365 : 397;
	int len = 0;
	for (int score_w = score_sum_; score_w != 0; score_w /= 10, len++);
	if (len == 0)len = 1;
	dstx = (flag(PLAYER1) ? 254 : 280) + 15 * (7 - len);

	for (int score_w = score_sum_; len; len--, dstx += 15)
	{
		int num = static_cast<int>(score_w / (pow((float)10, len - 1)));
		score_w %= (int)(pow((float)10, len - 1));

		if (flag(PLAYER1))
		{
			assets->score_1p[num].setPosition(dstx, dsty);
			assets->score_1p[num].draw();
		}
		else
		{
			assets->score_2p[num].setPosition(dstx, dsty);
			assets->score_2p[num].draw();
		}
	}

	// 連鎖数を表示
	// 連鎖が起きた一番下の一番右の座標 →chain_right_bottom_
	if (flag(WAIT_RENSA))
	{
		float destx = static_cast<float>((flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + (P_SIZE * toX(chain_right_bottom_) - 1));
		float desty = static_cast<float>((flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + (P_SIZE * (12 - toY(chain_right_bottom_))));
		float koudan_x = destx, koudan_y = desty;

		static const float pai = 3.141592f;
		static const float pai_2 = pai / 2.0f;
		static const float pai_4 = pai_2 / 2.0f;
		static const float pai_3_7 = pai * 3.0f / 7.0f;

		float now_percentage = (float)chain_stage_ / (float)CHAINTIME;
		float percent = (float)chain_stage_ / ((float)CHAINTIME / 2.0f);

		desty -= (128.0f * sin(pai_2 * now_percentage));

		// 連鎖が起こった後の光の玉を表示する
		float start_radian = pai_3_7;
		float dx, dy;

		if (flag(PLAYER1))
			dx = ojama_ ? 127.0f : 478.0f;
		else
			dx = ojama_ ? 478.0f : 127.0f;

		dy = 17;

		// koudan_x = dx + sin(3π/7) * nより
		float n = (koudan_x - dx) / sin(start_radian);
		koudan_x = dx + n * sin(start_radian + (pai - start_radian) * (1 - sin(pai_2 + pai_2 * percent)));
		float y = dy - koudan_y;// yの必要な移動量
		koudan_y += y * (1 - sin(pai_2 + pai_2 * percent));

		if (koudan_y < dy)
		{
			koudan_x = dx;
			koudan_y = dy;
		}

		if (flag(PLAYER1))
		{
			assets->lightball_1p.setPosition((int)koudan_x, (int)koudan_y);
			assets->lightball_1p.draw();
			assets->chain_str.setPosition((int)destx, (int)desty);
			assets->chain_str.draw();
		}
		else
		{
			assets->lightball_2p.setPosition((int)koudan_x, (int)koudan_y);
			assets->lightball_2p.draw();
			assets->chain_str_2p.setPosition((int)destx, (int)desty);
			assets->chain_str_2p.draw();
		}
		destx -= 17;

		if (chain_ < 10)
		{
			if (flag(PLAYER1))
			{
				assets->score_1p[chain_].setPosition((int)destx, (int)desty);
				assets->score_1p[chain_].draw();
			}
			else
			{
				assets->score_2p[chain_].setPosition((int)destx, (int)desty);
				assets->score_2p[chain_].draw();
			}
		}
		else
		{
			if (flag(PLAYER1))
			{
				assets->score_1p[chain_ - 10].setPosition((int)destx, (int)desty);
				assets->score_1p[chain_ - 10].draw();
				destx -= 17;
				assets->score_1p[1].setPosition((int)destx, (int)desty);
				assets->score_1p[1].draw();
			}
			else
			{
				assets->score_2p[chain_ - 10].setPosition((int)destx, (int)desty);
				assets->score_2p[chain_ - 10].draw();
				destx -= 17;
				assets->score_2p[1].setPosition((int)destx, (int)desty);
				assets->score_2p[1].draw();
			}
		}
	}

	// お邪魔ぷよの表示
	// 本物の通ルール
	// 小ぷよ（1個分）
	// 大ぷよ（6個分）
	// 岩ぷよ（30個分）
	// キノコぷよ（200個分）
	// 星ぷよ（300個分）
	// 王冠ぷよ（400個分）
	// 振るお邪魔ぷよがある == 予告ぷよを表示しなくてはいけない
	if (ojama_)
	{
		static const int ojama_kind[6] = { 1, 6, 30, 200, 300, 400 };
		int ojama[6] = { 0 };
		int ojama_w = ojama_;

		for (int i = 5; i >= 0; i--)
		{
			ojama[i] = ojama_w / ojama_kind[i];
			ojama_w %= ojama_kind[i];
		}
		// ここを抜けた時点で、予告ぷよの種類がそれぞれ何個表示されるべきかがojama[i]に入っている

		// iは表示したい予告ぷよの最大値
		for (int i = 0; i < 6; i++)
		{
			int j;
			for (j = 5; j >= 0; j--)
			{
				if (ojama[j])
				{
					ojama[j]--;
					break;
				}
			}
			if (j == -1)break;

			int destx, desty;
			if (flag(PLAYER1))
			{
				destx = P1_F_BEGINX + P_SIZE * i;
				desty = P1_F_BEGINY - P_SIZE;
			}
			else
			{
				destx = P2_F_BEGINX + P_SIZE * i;
				desty = P2_F_BEGINY - P_SIZE;
			}
			assets->yokoku[j].setPosition(destx, desty);
			assets->yokoku[j].draw();
		}
	}
}

bool Field::canDelete()
{
	if (flag(VANISH))
	{
		clearFlag(VANISH);
		setFlag(RENSA);
		return true;
	}
	else
	{
		clearFlag(SET);
		return false;
	}
}

// 落とすぷよがあればflag_ | SLIDE　なければflag(~SLIDE
// SLIDEできなかったらfalseを返す
bool Field::slide()
{
	bool ret = false;
	clearFlag(SLIDE);

	for (int x = 1; x <= 6; x++)
	{
		for (int y = 1; y <= RANK_14; y++)
		{
			if (isEmpty(toSquare(x, y))) // 空白を見つけたら
			{
				// 仮にそこを頂点座標としておく。
				upper_y_[x] = Rank(y);

				// その上方向のぷよを探す
				do {
					++y;
				} while (y <= RANK_14 && isEmpty(toSquare(x, y)));

				if (y > RANK_14)// 上方向に何もなければ
					break;
				
				setFlag(SLIDE);
				ret = true;

				do {
					if (!isEmpty(toSquare(x, y)))
					{
						// 見つけたぷよを、空白の場所に移す
						field_[toSquare(x, y) + SQ_DOWN] = field_[toSquare(x, y)];
						field_[toSquare(x, y)] = EMPTY;

						// 最後にswapした場所が最上位のぷよ
						upper_y_[x] = Rank(y);
					}
					++y;
				} while (y <= RANK_14);
			}
			else
			{
				if (y == RANK_13)
					upper_y_[x] = RANK_14;				
			}
		}
	}

	return ret;
}

bool Field::vanish()
{
	// すでに消えないことがわかっている場所を記憶しておくためのbitboard
	Bitboard bb_allready(0, 0), bb_over4(0, 0);

	ChainElement ce(chain());

	// 消した色に応じたbitを1にしていって，後でpopcountで数える．
	int color_bit = 0;

	clearFlag(RENSA);

	if (chain() == 0)
	{
		for (int x = 1; x <= 6; x++)
			recul_y_[x] = RANK_2;
	}

	for (int x = 1; x <= 6; x++)
	{
		const int wy = min3(recul(x - 1) ,recul(x), recul(x + 1));

		// recul_y_は連鎖が起こった一番↓のy座標が入っているので、wy-1ではなくwyでつかえばいいような気もするが、
		// 実際にwyでやると2つ繋がっている場所を見逃すので、wy-1で使用している
		for (int y = wy - 1; y < upper(x); y++)
		{
			const Square sq = toSquare(x, y);
			const Connect c = connect(sq);

			// 連鎖がおこるときは、最低でもひとつは二つ以上つながっているぷよが存在するはず．
			if(bb_allready.isNotSet(sq) && (c & (c - 1)))
			{
				// つながっている個数
				Bitboard bb = bbSameColor(sq);
				bb_allready |= bb;
				const int count = bb.count();
				
				if (count >= 4)
				{
					bb_over4 |= bb;
					color_bit |= 1 << colorToColorType(color(sq));

					// 連結ボーナスを計算
					ce.connectPlus(count);
				}
			}
		}
	}

	const bool is_ren = bb_over4.isTrue();

	if(is_ren)
	{
		// 同時消し色数ボーナス
		ce.setColorBonus(Bitop::popCount(color_bit));

		// 個数をカウント
		ce.setPuyoNum(bb_over4.count());

		// 消える位置のbitboardはあとで使うのでコピー渡しする
		deletePuyo<true>(Bitboard(bb_over4));

		setFlag(RENSA | SLIDE);
		
		// connectだけすでにボーナス計算済み
		int bonus = ce.chainScore();

		// 1連鎖ならボーナス点0の連鎖もありえる．
		if (bonus == 0)
			bonus = 1;

		// 全消し
		if (flag(ALLCLEAR))
		{
			score_ += RATE * 30;
			clearFlag(ALLCLEAR);
		}
		
		chain_++;
		score_ += bonus;
		score_sum_ += bonus;

		// 連鎖が起きた一番右下の座標を記憶しておく
		// 連鎖が起きた場所はdammyが2になっているところ。

		// 一番左上にしておく
		chain_right_bottom_ = toSquare(1, 13);

		for (int x = 1; x <= 6; x++)
		{
			for (int y = 1; y <= 12; y++)
			{
				const Square sq = toSquare(x, y);

				if (bb_over4.isSet(sq) && y <= toY(chain_right_bottom_))
				{
					if (y == toY(chain_right_bottom_))
					{
						if (x > toX(chain_right_bottom_))
							chain_right_bottom_ = toSquare(x, y);
					}
					else
						chain_right_bottom_ = toSquare(x, y);
				}
			}
		}
	}
	else
	{
		chain_ = 0;
		clearFlag(SET | SLIDE);
	}

	return is_ren;
}

// 接地完了時、TIGIRIをはずし、SETをセット
// TIGIRIが外れたらfalseが戻る（fall失敗的な意味で）
bool Field::fall()
{
	field_[tigiri_ + SQ_DOWN] = field_[tigiri_];
	field_[tigiri_] = EMPTY;
	--tigiri_;

	if (!isEmpty(tigiri_ + SQ_DOWN))
	{
		// もうこれ以上ちぎれたぷよが落下できないので、位置を確定させる
		++upper_y_[toX(tigiri_)];
		setConnect(tigiri_);
		clearFlag(TIGIRI);// 設置完了
		setFlag(SET);
		return false;
	}
	return true;
}

void Field::ojamaFall(Field &enemy)
{
	int rand_ojama[6] = { 1, 2, 3, 4, 5, 6 };

	// 相手が連鎖中や、自分のお邪魔が振っている最中はおじゃまぷよは振らない
	if (!flag(OJAMA_WILLFALL) || (!flag(OJAMA_FALLING) && !ojama_) || flag(OJAMA_FALLING) && !ojama_buf_)
	{
		return;
	}
	else
	{
		if (!flag(OJAMA_FALLING))
		{
			if (ojama_ >= 30)
			{
				ojama_buf_ = 30;
				ojama_ -= 30;// この関数が呼び出されたはじめの一回の時点でのお邪魔ぷよを降らせる
				enemy.score_max_ -= RATE * 30;
			}
			else
			{
				ojama_buf_ = ojama_;
				ojama_ = 0;
				enemy.score_max_ -= RATE * ojama_buf_;
			}
			setFlag(OJAMA_FALLING);// おじゃまぷよ落下中フラグ
		}

		if (ojama_buf_ >= 6)
		{
			ojama_buf_ -= 6;

			for (int x = 1; x <= 6; x++)
				field_[toSquare(x, 14)] = OJAMA;
		}
		else// おじゃまぷよが６個未満のとき
		{
			// shuffle
			for (int n = 0; n < 6; n++)
			{
				int r;

				if (flag(REPLAY_MODE))
					r = assets->ojama_history.pop();
				else
				{
					r = assets->ojama_rand_[ojama_rand_id_];
					ojama_rand_id_ = (ojama_rand_id_ + 1) & 127;

					if (!flag(STATIC_MAKING))
						assets->ojama_history.push(r);
				}
				std::swap(rand_ojama[n], rand_ojama[r]);
			}
			int o = ojama_buf_;

			for (int x = 0; x < o; x++)
			{
				field_[toSquare(rand_ojama[x], 14)] = OJAMA;
				ojama_buf_--;
			}
			setFlag(OJAMAOK);
		}

		if (ojama_buf_ <= 0)
		{
			assert(ojama_buf_ == 0);// 0になっているはず
			setFlag(OJAMAOK);
		}
	}
}

// 硬直時間と、おじゃまぷよ、ゲームオーバー処理
int Field::wait(Field &enemy)
{
	if (flag(WAIT)) // 硬直時間
	{
		if (flag(WAIT_OJAMA))
		{
			if (wait_ < NEXTTIME)
			{
				return 1;
			}
			else if (wait_ < NEXTTIME + 1)
			{
				if (!flag(OJAMAOK))
					ojamaFall(enemy);	

				if (slide())
				{
					assert(enemy.ojama_ == 0);
					wait_ -= FALLTIME - 1;
					return 1;
				}
			}
			if (!ojama_)// おじゃまぷよが残っていない
				clearFlag(OJAMA_WILLFALL);

			if (flag(OJAMA_RESERVE))// お邪魔ぷよがふっている最中に相手の連鎖が終わった場合
			{
				assert(ojama_);// ここにくるということはおじゃまぷよは残っているはず
				clearFlag(OJAMA_RESERVE);
				setFlag(OJAMA_WILLFALL);
			}
			clearFlag(WAIT_OJAMA | OJAMAOK | OJAMA_FALLING);

			if (isDeath())// 連鎖が終わっても、[3][12]にぷよがある
				return -1;

			// 14だんめのぷよを消去する
			for (int x = 1; x <= 6; x++)
				field_[toSquare(x, 14)] = EMPTY;

			// 一手一手を入力していくモードの受付タイミングは、このZURU_OKが発行されたタイミングである．
			setFlag(ZURU_OK);
			return 1;
		}
		else if (flag(ZURU_OK))
		{
			clearFlag(ZURU_OK);

			// AIはこのWAIT_NEXTが発行されたときに一回だけ思考する。
			// 多少時間がかかっても問題ではない作業はここでやってしまう。
			setFlag(WAIT_NEXT);

			if (!flag(ZURU_MODE) || flag(PLAYER2))
				tumoReload();

			aiInit();
			assert(examine());
			return 1;
		}
		else if (flag(WAIT_NEXT))
		{
			clearFlag(WAIT_NEXT);
			r_count_ = l_count_ = r_rotate_count_ = l_rotate_count_ = down_count_ = 0;// AI用の仕様（ズルイ）
			tigiri_= Square(0);
			remain_ = Square(0);

			// なぜPutpuyoを分けるかというと、AIを呼び出す際に(3,12)にぷよが置かれていると不都合なため
			putTumo(current());
		}
		else if (flag(WAIT_SET))
		{
			if (wait_ < SETTIME)
			{
				puyon_stage_ = wait_;
				return 1;
			}

			clearFlag(WAIT_SET);
		}
		else if (flag(WAIT_RENSA))
		{
			if (wait_ < CHAINTIME)
			{
				chain_stage_ = wait_;

				if (wait_ == (CHAINTIME / 2 + 5))
				{
					if (chain_ >= 2)
					{
						int c = chain_;

						if (chain_ >= 5)
							c = 5;

						// 連鎖音（ドーンとかきらきらとか）
						if (!flag(STATIC_MAKING))
							assets->chain_effect[c - 2].play();
					}
				}
				return 1;
			}

			for (int x = 1; x <= 6; x++)
				for (int y = 1; y <= upper(x); y++)
					delete_mark_[toSquare(x, y)] = 0;

			clearConnect();
			clearFlag(WAIT_RENSA);
		}
		else if (flag(WAIT_TIGIRISET))
		{
			if (wait_ < TIGIRITIME)
			{
				puyon_stage_ = wait_;
				return 1;
			}

			clearFlag(WAIT_TIGIRISET);
			setFlag(WAIT_TIGIRI);
			wait_ = 0;
			return 1;
		}
		else if (flag(WAIT_TIGIRI))
		{
			if (wait_ < FALLTIME)
			{
				drop_offset_ += (float)P_SIZE / (float)FALLTIME;
				return 1;
			}
			drop_offset_ = 0;
			clearFlag(WAIT_TIGIRI);
		}
		else if (flag(WAIT_SLIDE))
		{
			if (wait_ < FALLTIME)
			{
				drop_offset_ += (float)P_SIZE / (float)FALLTIME;
				return 1;
			}

			drop_offset_ = 0;
			clearFlag(WAIT_SLIDE);
		}
		else if (flag(WAIT_DROP))
		{
			if (wait_ < FALLTIME)
			{
				drop_offset_ += (float)P_SIZE / ((float)FALLTIME);
				return 1;
			}
			clearFlag(WAIT_DROP);
			setFlag(DROP);
			drop_offset_ = 0;

			// wait_を元に戻す
			// 自由落下のオフセット値の整合性を取るため。
			wait_ = fall_w_;
		}
		else
			assert(false);
	}
	return 0;
}

// gameの処理
bool Field::procedure(Field &enemy, Operate *ope)
{
	int rc = wait(enemy);

	if (rc == 1){ ; }// 何もしない
	else if (rc == -1)
	{
		// ゲーム終了
		return false;
	}
	else if (flag(TIGIRI))// ちぎり処理
	{
		wait_ = 0;
		setFlag(WAIT_TIGIRI);

		if (!fall())// 1個だけ落とす
		{
			if (!flag(STATIC_MAKING))
				assets->drop.play();

			setFlag(WAIT_SET);
		}
	}
	else if (flag(SET))// 設置完了して、弾むエフェクトを実行した後にここに来る
	{
		wait_ = 0;

		if (flag(SLIDE))// 連鎖後、ぷよ落下中
		{
			setFlag(WAIT_SLIDE);

			if (!slide())// スライド完了
			{
				setConnectRecul();
				clearFlag(WAIT_SLIDE);
				setFlag(WAIT_SET);

				// 消えるときのぷよに印をつけておく
				deleteMark();
			}
		}
		else
		{
			if (flag(RENSA))// 連鎖途中
			{
				if (vanish())// 連鎖が起きたとき
				{
					// 6連鎖目までは普通の連鎖ボイス。
					// 7連鎖以降は同じ連鎖ボイスを使う。
					int c = chain_ - 1;

					if (c >= 7)
						c = 6;

					if (!flag(STATIC_MAKING))
					{
						if (flag(PLAYER1))
						{
							assets->chain[c].play();
							assets->voice_1p[c].play();
						}
						else
						{
							assets->chain[c].play();
							assets->voice_2p[c].play();
						}
					}
					setFlag(WAIT_RENSA); // 連鎖後の硬直時間	
					chain_stage_ = wait_;

					offseting(enemy);// おじゃまぷよが予約されているなら、相殺する
				}
				else// 連鎖終了
				{
					// bonusInit();
					chain_max_ = 0;

					// もし相手がおじゃまぷよ落下中なら
					if (enemy.flag(OJAMA_FALLING))
					{
						if (enemy.ojama_)
							enemy.setFlag(OJAMA_RESERVE);// おじゃま落下が終わった後降るフラグ
					}
					else
					{
						// おじゃまぷよが降っている間は、新たなおじゃまぷよは送らない
						// 相手におじゃまが振らない場合はojama_フラグは立てててはいけない（立てると、お邪魔が振るまでフラグが消えないから）
						if (enemy.ojama_)
							enemy.setFlag(OJAMA_WILLFALL);// 自分の連鎖が終わったら、相手に次に尾じゃまぷよが降るフラグを立てておく
						else
							enemy.clearFlag(OJAMA_WILLFALL);
					}
					if (isEmpty())// 全消し
					{
						setFlag(ALLCLEAR);

						if (!flag(STATIC_MAKING))
							assets->allclear.play();
					}
				}
			}
			else if (canDelete())// 連鎖でも、ぷよが落ちている最中でもない
			{
				if (!flag(REPLAY_MODE))
				{
					// この処理はゲームには関係ない。大連鎖のログを残す処理
					Field f(*this);

					//Key w_key = f.key();
					f.keyInit();

					//assert(w_key == f.key());

					while (f.deleteMin());

					// AI用に連鎖数、点数を記憶しておく	
					// もはや初代AIはふるいのでいらないかも。
					score_max_ += f.score();
					enemy.score_max_ -= f.score();
					chain_max_ = f.chain_max_;

					// rensa.txtというファイルにログを残す．
					if (chain_max_ > 11)
						simuraterConvert(f.score());
				}

				// 消えるときのぷよに印をつけておく
				deleteMark();

				setFlag(WAIT_SET);
				return true;
			}

			if (!flag(RENSA))// 連鎖が起きなかった場合
			{
				// 新しいぷよを出現させる
				setFlag(WAIT_OJAMA);
			}
		}
	}
	else// 連鎖の途中ではない
	{
		if (wait_ % FALLTIME == 0)
		{
			int p = processInput(ope);

			if (p & DOWN)// ↓ボタンが押された時（それ以外なら押された方向に移動、回転をする）
			{
				score_++;
				score_sum_++;

				// 2つとも↓が空なら、少しずつずらして表示する処理をする
				if (isEmpty(current().psq() + SQ_DOWN2) && isEmpty(current().csq() + SQ_DOWN2))
				{
					setFlag(WAIT_DROP);
					fall_w_ = wait_;
					wait_ = 0;
				}
				else
					wait_ = FREEDROPTIME;
			}

			if (p & RIGHT)
				move_offset_ -= P_SIZE / 2;
			else if (p & LEFT)
				move_offset_ += P_SIZE / 2;

			// TODO:回転時の描画が荒い．
			if (p & R_ROTATE)
			{
				if(current().rotate() == 0)
				{
					rotate_offset_x_ -= 24;
					rotate_offset_y_ += 24;
				}
				else if(current().rotate() == 1)
				{
					rotate_offset_x_ -= 24;
					rotate_offset_y_ -= 24;
				}
				else if(current().rotate() == 2)
				{
					rotate_offset_x_ += 24;
					rotate_offset_y_ -= 24;
				}
				else if(current().rotate() == 3)
				{
					rotate_offset_x_ += 24;
					rotate_offset_y_ += 24;
				}
			}
			else if (p & L_ROTATE)
			{
				if(current().rotate() == 0)
				{
					rotate_offset_x_ += 24;
					rotate_offset_y_ += 24;
				}
				else if(current().rotate() == 1)
				{
					rotate_offset_x_ -= 24;
					rotate_offset_y_ += 24;
				}
				else if(current().rotate() == 2)
				{
					rotate_offset_x_ -= 24;
					rotate_offset_y_ -= 24;
				}
				else if(current().rotate() == 3)
				{
					rotate_offset_x_ += 24;
					rotate_offset_y_ -= 24;
				}
			}
		}
		if ((wait_ % FREEDROPTIME == 0 && !flag(WAIT_DROP)) || flag(DROP))
		{
			dropTumo();

			if (!flag(DROP))
				freedrop_offset_ = 0;

			clearFlag(DROP);

			if (flag(SET))
			{
				wait_ = 0;
				setFlag(WAIT_SET);

				if (!flag(STATIC_MAKING))
					assets->drop.play();

				puyon_stage_ = 0;
			}
			else if (flag(TIGIRI))
			{
				wait_ = 0;
				setFlag(WAIT_TIGIRISET);

				if (!flag(STATIC_MAKING))
					assets->drop.play();

				puyon_stage_ = 0;
			}
		}
		else if (!flag(WAIT_DROP))
		{
			// 下ボタンが押されていないことによる自由落下によるオフセット
			freedrop_offset_ += (float)P_SIZE / (float)FREEDROPTIME;
		}
	}
	wait_++;

	// 現在のフィールドの状態をメモリに描画する
	if (!flag(STATIC_MAKING))
		show();

	return true;
}

int Field::processInput(Operate *ope)
{
	assert(flag(PLAYER1 | PLAYER2) || ope);

	// 同時に押すことが許されない処理だけ、if-elseifにしている
	int	operate_bit = 0;
	int ret = 0;
	Tumo n = current();
	bool b = true;

	// 操作の仕様
	// 回転キーが押しっぱなしでも回転しない．
	// 横移動は、右、左どちらかの移動のみ受け付ける
	// フィールド上で回転できない場所は14段目での回転のみ（15段目に壁が存在する）
	
	if (flag(REPLAY_MODE))
	{
		if (flag(PLAYER1))
			operate_bit = assets->operate_history_1p.pop();
		else
			operate_bit = assets->operate_history_2p.pop();
	}
	else
	{
		if (flag(PLAYER1))// 1p用
		{
			if (flag(PLAYER_AI) || flag(ZURU_MODE))
				operate_bit = ope->pop();
			else
			{
					// キー入力に応じて処理
					if (CheckHitKey(KEY_INPUT_LEFT))	operate_bit |= LEFT;
			   else if (CheckHitKey(KEY_INPUT_RIGHT))	operate_bit |= RIGHT;
					if (CheckHitKey(KEY_INPUT_X))		operate_bit |= R_ROTATE;
			   else	if (CheckHitKey(KEY_INPUT_Z))		operate_bit |= L_ROTATE;
			   		if (CheckHitKey(KEY_INPUT_DOWN))	operate_bit |= DOWN;
			}
		}
		else if (flag(PLAYER2))// 2p用
		{
			if (flag(PLAYER_AI))
				operate_bit = ope->pop();
			else
			{
				if (CheckHitKey(KEY_INPUT_A))	operate_bit |= LEFT;
		   else if (CheckHitKey(KEY_INPUT_D))	operate_bit |= RIGHT;
				if (CheckHitKey(KEY_INPUT_G))	operate_bit |= R_ROTATE;
	       else if (CheckHitKey(KEY_INPUT_F))	operate_bit |= L_ROTATE;
				if (CheckHitKey(KEY_INPUT_S))	operate_bit |= DOWN;
			}
		}

		// 操作の履歴を取っておく．（リプレイ再生のため）
		if (flag(PLAYER1))
			assets->operate_history_1p.push(operate_bit);
		else
			assets->operate_history_2p.push(operate_bit);
	}
	if (operate_bit & LEFT)// もし左ｷｰが押されたら
	{
		if (l_count_ == 0 || l_count_ > 1)
		{
			n.left();

			if (putTumo(n))
			{
				assets->move.play();
				current_ = n;
				ret |= LEFT;
			}
			else
				n.right();
		}

		l_count_++;
		r_count_ = 0;
	}
	else if (operate_bit & RIGHT)// もし→ｷｰが押されたら
	{
		if (r_count_ == 0 || r_count_ > 1)
		{
			n.right();

			if (putTumo(n))
			{
				assets->move.play();
				current_ = n;
				ret |= RIGHT;
			}
			else
				n.left();
		}

		r_count_++;
		l_count_ = 0;
	}
	else
		r_count_ = l_count_ = 0;

	if (operate_bit & R_ROTATE)// 右回転
	{
		if (r_rotate_count_ == 0)
		{
			n.rightRotate();

			if (!putTumo(n))
			{
				// 回転させたぷよが横向きである＝現在のぷよが縦向き
				if (n.isSide())
				{
					// 回転できなかったら左右どちらかが壁になっている。
					// ぷよが右向き（rotate == 1)なら左に移動、左向き(rotate == 3)なら右に移動
					n.addx(File(n.rotate() - 2));

					if (!putTumo(n))
					{
						// それでも置けなかったら、今度は左右どちらとも壁になっているので、2回転させる
						n.addx(-File(n.rotate() - 2));

						// 右上が開いている
						if (isEmpty(n.psq() + SQ_RIGHT + SQ_UP) && n.rotate() == 1)
						{
							n.up();
							n.rightRotate();
						}
						else
							n.rightRotate();

						if (!putTumo(n))
						{
							n.up();

							if (!putTumo(n))// ここまできて置けなかったら、15段目の壁にぶつかっているので操作しない
								b = false;
						}
					}
				}

				else// 横向き
				{
					n.up();

					if(n.py() == prev_y_)
					{
						up_limit_++;
					}
					else
					{
						up_limit_ = 0;
						prev_y_ = n.py();
					}
					if (!putTumo(n))// ここまできて置けなかったら、15段目の壁にぶつかっているので操作しない
						b = false;
				}
			}
			if (b)
			{
				current_ = n;
				assets->rotate.play();
				ret |= R_ROTATE;
			}
			else
				b = true;
		}
		r_rotate_count_++;
	}
	else if (operate_bit & L_ROTATE)// 左回転
	{
		if (l_rotate_count_ == 0)
		{
			n.leftRotate();

			if (!putTumo(n))// 置けないなら
			{
				if (n.isSide())// 現在のぷよが縦向き
				{
					// 回転できなかったら左右どちらかが壁になっている
					n.addx(File(n.rotate() - 2));

					if (!putTumo(n))
					{
						// それでも置けなかったら、今度は左右が壁になっているので、2回転させる
						n.addx(-File(n.rotate() - 2));

						if (isEmpty(n.psq() + SQ_LEFT + SQ_UP) && n.rotate() == 3)
						{
							// 左上が開いている
							n.up();
							n.leftRotate();
						}
						else
							n.leftRotate();

						if (!putTumo(n))
						{
							n.up();

							if (!putTumo(n))
								b = false;
						}
					}
				}
				else
				{
					n.up();

					if(n.py() == prev_y_)
					{
						up_limit_++;
					}
					else
					{
						up_limit_ = 0;
						prev_y_ = n.py();
					}

					if (!putTumo(n))
						b = false;
				}
			}
			if (b)
			{
				current_ = n;
				assets->rotate.play();
				ret |= L_ROTATE;
			}
			else
				b = true;
		}
		l_rotate_count_++;
	}
	else
		r_rotate_count_ = l_rotate_count_ = 0;

	if (operate_bit & DOWN)// もし↓ｷｰが押されたら
	{
		ret |= DOWN;// 下に移動するのではなく、一定時間ごとに下に下りていく処理（BlockDown)を呼ぶためにtrueを返すようにしておく
		down_count_++;
	}
	else
		down_count_ = 0;

	if(up_limit_ > 4)
	{
		up_limit_ = 0;
		ret |= DOWN;
	}

	return ret;
}

// 静かな局面になるまで、局面を進める(WAIT_NEXTが発行されるまで)
// もし死んでいる局面だった場合は-1を返す
// countは+、もしくは-の場合はFALLTIMEの倍数になるので死んでいる局面の場合以外は-1になることはない。
int Field::generateStaticState(Field& enemy)
{
	int count = 0;

	// 操作が必要でない局面まで進んでいた場合
	if (flag(SET | TIGIRI | RENSA | SLIDE | WAIT_OJAMA | ZURU_OK))
	{
		// STATIC_MAKINGをセットした状態でprocedureを呼び出せば音や画面表示、リプレイロギングを行わない
		setFlag(STATIC_MAKING);

		// WAIT_NEXTが発行されるまでゲームを進める
		while (!flag(WAIT_NEXT))
		{
			count++;

			if (!procedure(enemy))
				return -1;
		}
		clearFlag(STATIC_MAKING);
		bonusInit();
		assert(examine());
	}
	else
	{
		// まだ操作中かもしれないので、操作中のぷよを消す
		setEmpty(current());

		// 概算で落下までにかかる時間を算出しておく
		// 12段目からの距離*1マス落とすのにかかる時間を自分の残り時間から引いておく
		// なぜなら相手はその分だけ落下が早く終わるから。
		count -= (12 - std::min(current().py(), current().cy())) * FALLTIME;

		// これも必要ないかもしれないが念のため
		bonusInit();
	}

	return count;
}

void Field::deleteMark()
{
	Field f(*this);

	if (f.vanish())
	{
		for (int x = 1; x <= 6; x++)
		{
			for (int y = 1; y < upper(x); y++)
			{
				const Square sq = toSquare(x, y);

				if (f.isEmpty(sq) && !isEmpty(sq))
				{
					// x,yはきえるぷよであるということでマークをつける
					// VANISHは再利用した
					delete_mark_[sq] = color(sq);
					delete_mark_[sq] |= VANISH;
				}
			}
		}
	}
}

// 面白そうな連鎖ならぷよシミュレータで読み込める形のURLを生成する
int Field::simuraterConvert(int score)
{
	FILE *fp;
	errno_t error;

	if ((error = fopen_s(&fp, "rensa.txt", "a+")) != 0)
		return 0;

	fprintf(fp, "http://www.puyop.com/s/");

	int conv_col[8] = { 0, -1, 1, 2, 3, 4, 5, 6 };
	char buf[128] = "";

	int colBit = 0;
	int cnt = 0;
	const char sin[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	// 0 空 1 赤 2 緑 3 青 4 黄 5 紫 6 お邪魔
	bool b = false;

	for (int y = 13; y >= 1; y--)
	{
		for (int x = 1; x <= 6; x++)// 左上から順番に
		{
			const Square sq = toSquare(x, y);

			if (isEmpty(sq) && !b)
				continue;
			else
				b = true;

			if (x % 2)
			{
				colBit = 0;
				colBit |= conv_col[color(sq) >> 4] << 3;
			}
			else
				colBit |= conv_col[color(sq) >> 4];

			if (x % 2 == 0)
				buf[cnt++] = sin[colBit];
		}
	}
	buf[cnt] = '\0';

	fprintf(fp, "%s", buf);
	fprintf(fp, " %drensa %dscore\n", chain_max_, score);

	fclose(fp);
	return 1;
}
