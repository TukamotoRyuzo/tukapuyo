#include"move.h"
#include"field.h"
#include<algorithm>
#include"gamemode.h"

// 確実に、この手が合法かどうか調べる
// 少し重い
bool Move::isLegal(LightField f)
{
	Move buf[22];
	int movemax = f.generateMoves(buf);
	for (int i = 0; i < movemax; i++)
	{
		if (m_ == buf[i].m_)
		{
			return true;
		}
	}
	return false;
}

// 大体合法かどうか調べる
// これでもほぼ確実に調べることができる
bool Move::isLegalAbout(const LightField& f) const 
{

	const Square psq = this->psq();
	const Square csq = this->csq();

	// 手が示す場所が与えられた局面において空かどうか
	if (!f.isEmpty(psq) || !f.isEmpty(csq))
		return false;

	// 今から配置するぷよの一個↓が空ではないことを確認する
	// 回転数が縦
	if (toX(psq) == toX(csq))
	{
		const Square minsq = std::min(psq, csq);

		if (f.isEmpty(minsq + SQ_DOWN))
			return false;
	}
	// 回転数が横
	else
	{
		if (f.isEmpty(psq + SQ_DOWN) || f.isEmpty(csq + SQ_DOWN))
			return false;
	}
	return true;
}

std::string Move::toString(LightField f, const int depth) const
{
	const char col[5] = { 'R', 'G', 'B', 'Y', 'P' };
	ColorType pc = colorToColorType(f.getDepthTumo(depth).pColor());
	ColorType cc = colorToColorType(f.getDepthTumo(depth).cColor());
	const char c[] = { col[pc], col[cc], '\0' };
	return squareToStr(psq()) + squareToStr(csq()) + std::string(c);
}

// 目的の位置にぷよを落とすために、操作をキューにつめていく
// 完全なるしらみつぶしによるハードコーディングで面白くもなんともない。
void Operate::generate(const Move dest, const Field &f)
{
	int necX, necY, necR;// 必要な横移動と下移動、回転数
	int n = 0;

	int operate_buf[40];

	const int dpx = toX(dest.psq());
	const int dcx = toX(dest.csq());
	const int dpy = toY(dest.psq());
	const int dcy = toY(dest.csq());
	necX = dpx - 3;// 3:現在の場所
	necY = 12 - std::max(dpy, dcy);

	if (dcx == dpx)
	{
		if (dpy < dcy)
			necR = 0;
		else
			necR = 2;
	}
	else
	{
		if (dpx < dcx)
			necR = 1;
		else
			necR = 3;
	}


	if (necY < 0) 
		necY = 0;

	for (int i = 0; i < 40; i++)
	{
		operate_buf[i] = DOWN;// まず↓ボタンは押しっぱなしにする

		if (necX > 0 && i % 2 == 0 && i < necX * 2)
			operate_buf[i] |= RIGHT;
		else if (necX < 0 && i % 2 == 0 && i < abs(necX) * 2)
			operate_buf[i] |= LEFT;
	}

	// 右移動
	if (necX > 0)
	{
		if (necX == 3)
		{
			if (!f.isEmpty(toSquare(4, 8)))
			{
				operate_buf[0] &= ~DOWN;

				if (!f.isEmpty(toSquare(4, 9)))
				{
					operate_buf[1] &= ~DOWN;

					if (!f.isEmpty(toSquare(4, 10)))
					{
						operate_buf[2] &= ~DOWN;

						if (!f.isEmpty(toSquare(4, 11)))
							operate_buf[3] &= ~DOWN;
					}
				}
			}
			if (!f.isEmpty(toSquare(5, 7)))
			{
				operate_buf[0] &= ~DOWN;

				if (!f.isEmpty(toSquare(5, 8)))
				{
					operate_buf[1] &= ~DOWN;
					operate_buf[2] &= ~DOWN;

					if (!f.isEmpty(toSquare(5, 9)))
						operate_buf[3] &= ~DOWN;
				}
			}
		}
		else if (necX == 2)
		{
			if (!f.isEmpty(toSquare(4, 9)))
			{
				operate_buf[0] &= ~DOWN;

				if (!f.isEmpty(toSquare(4, 10)))
				{
					operate_buf[1] &= ~DOWN;

					if (!f.isEmpty(toSquare(4, 11)))
						operate_buf[2] &= ~DOWN;
				}
			}
			if (!f.isEmpty(toSquare(5, 9)))
			{
				operate_buf[0] &= ~DOWN;
				if (!f.isEmpty(toSquare(5, 10)))
				{
					operate_buf[1] &= ~DOWN;
					operate_buf[2] &= ~DOWN;

					if (!f.isEmpty(toSquare(5, 11)))
						operate_buf[3] &= ~DOWN;
				}
			}
		}


		if (necR == 3)
		{
			if (!f.isEmpty(toSquare(3, 10)))
			{
				operate_buf[0] &= ~DOWN;

				if (!f.isEmpty(toSquare(3, 11)))
					operate_buf[1] &= ~DOWN;
			}
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			if (!f.isEmpty(toSquare(3, 10)))
			{
				operate_buf[0] &= ~DOWN;

				if (!f.isEmpty(toSquare(3, 11)))
					operate_buf[1] &= ~DOWN;
			}
			if (necX == 3)
			{
				if (necY == 0)
				{
					operate_buf[4] |= L_ROTATE;
					operate_buf[6] |= L_ROTATE;
				}
				else
				{
					operate_buf[0] |= L_ROTATE;
					operate_buf[2] |= L_ROTATE;
				}
			}
			else
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}

	}
	// 左移動
	else if (necX < 0)
	{
		if (!f.isEmpty(toSquare(2, 10)))
		{
			operate_buf[0] &= ~DOWN;

			if (!f.isEmpty(toSquare(2, 11)))
				operate_buf[1] &= ~DOWN;
		}

		if (necR == 3)
			operate_buf[0] |= L_ROTATE;
		else if (necR == 2)// 左移動時は左回転のほうが安全
		{
			if (!f.isEmpty(toSquare(3, 10)))
			{
				operate_buf[0] &= ~DOWN;
				operate_buf[1] &= ~DOWN;
			}
			if (necX == -2)
			{
				operate_buf[0] |= R_ROTATE;
				operate_buf[2] |= R_ROTATE;
			}
			else
			{
				operate_buf[0] |= R_ROTATE;
				operate_buf[2] |= R_ROTATE;
			}
		}
		else if (necR == 1)
		{
			if (!f.isEmpty(toSquare(3, 10)))
			{
				operate_buf[0] &= ~DOWN;

				if (!f.isEmpty(toSquare(3, 11)))
					operate_buf[1] &= ~DOWN;
			}
			operate_buf[0] |= R_ROTATE;
		}
	}

	else// 3列目におく場合
	{
		if (necR == 3)
			operate_buf[0] |= L_ROTATE;
		else if (necR == 2)
		{
			// 左右が詰まっている場合は、2回転は1回転ですむ
			if (!f.isEmpty(toSquare(4, 12)) && !f.isEmpty(toSquare(2, 12)))
			{
				operate_buf[0] |= R_ROTATE;
			}
			else if (!f.isEmpty(toSquare(4, 12)) && f.isEmpty(toSquare(2, 12)))
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
				operate_buf[0] &= ~DOWN;
				operate_buf[1] &= ~DOWN;
			}
			else if (!f.isEmpty(toSquare(2, 12)) && f.isEmpty(toSquare(4, 12)))
			{
				operate_buf[0] |= R_ROTATE;
				operate_buf[2] |= R_ROTATE;
				operate_buf[0] &= ~DOWN;
				operate_buf[1] &= ~DOWN;
			}
			else if (!f.isEmpty(toSquare(4, 10)) || !f.isEmpty(toSquare(2, 10)))
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
				operate_buf[0] &= ~DOWN;
				operate_buf[1] &= ~DOWN;
			}
			else
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
		}
		else if (necR == 1)
		{
			if (!f.isEmpty(toSquare(3, 11)))
			{
				operate_buf[0] &= ~DOWN;
				operate_buf[1] &= ~DOWN;
			}
			operate_buf[0] |= R_ROTATE;
		}
	}

	// ここまでで、DOWNがいくつあるか確かめる
	int downCount = 0;
	int i;

	for (i = 0; i < 40 && downCount < necY; i++)
		if (operate_buf[i] & DOWN)
			downCount++;

	for (; i < 40; i++)
		if ((i < 6 && necX == 3) || (i < 4 && (necX == 2 || necX == -2)) || i < 2 && necR == 2)
			operate_buf[i] &= ~DOWN;

	// 横移動
	while (necX)
	{
		if (necX < 0)
		{
			// 左方向の移動
			operate_buf[n++] |= LEFT;
			n++;// 連続する移動は受け付けられないので
			necX++;
		}
		else
		{
			operate_buf[n++] |= RIGHT;
			n++;
			necX--;
		}
	}

	clear();

	for (int i = 0; i < 40; i++)
		queue_.push(operate_buf[i]);
}

// 低レベル操作関数
void Operate::easyGenerate(const Move dest, const Field &f)
{
	int necX, necY, necR;// 必要な横移動と下移動、回転数
	int n = 0;

	int operate_buf[1000] = {0};

	const int dpx = toX(dest.psq());
	const int dcx = toX(dest.csq());
	const int dpy = toY(dest.psq());
	const int dcy = toY(dest.csq());

	necX = dpx - 3;// 3:現在の場所
	necY = 12 - std::max(dpy, dcy);

	if (dcx == dpx)
	{
		if (dpy < dcy)
			necR = 0;
		else
			necR = 2;
	}
	else
	{
		if (dpx < dcx)
			necR = 1;
		else
			necR = 3;
	}

	if (necY < 0) 
		necY = 0;

	// 右移動
	if (necX > 0)
	{
		for(int i = 0; i < necX; i++)
			operate_buf[i*2] |= RIGHT;

		if (necR == 3)
		{
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			operate_buf[0] |= L_ROTATE;
			operate_buf[2] |= L_ROTATE;
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}

	}
	// 左移動
	else if (necX < 0)
	{
		for(int i = 0; i < abs(necX); i++)
			operate_buf[i*2] |= LEFT;

		if (necR == 3)
		{
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			operate_buf[0] |= R_ROTATE;
			operate_buf[2] |= R_ROTATE;
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}
	}

	else// 3列目におく場合
	{
		if (necR == 3)
		{
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			// 左右が詰まっている場合は、2回転は1回転ですむ
			if (!f.isEmpty(toSquare(4, 12)) && !f.isEmpty(toSquare(2, 12)))
			{
				operate_buf[0] |= R_ROTATE;
			}
			else if (!f.isEmpty(toSquare(4, 12)) && f.isEmpty(toSquare(2, 12)))
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
			else if (!f.isEmpty(toSquare(2, 12)) && f.isEmpty(toSquare(4, 12)))
			{
				operate_buf[0] |= R_ROTATE;
				operate_buf[2] |= R_ROTATE;
			}
			else if (!f.isEmpty(toSquare(4, 10)) || !f.isEmpty(toSquare(2, 10)))
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
			else
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}
	}

	for(int i = 5; i < 1000; i += 5)
	{
		operate_buf[i] |= DOWN;
	}

	clear();

	for (int i = 0; i < 400; i++)
		queue_.push(operate_buf[i]);
	
}

void Operate::veryEasyGenerate(const Move dest, const Field &f)
{
	int necX, necY, necR;// 必要な横移動と下移動、回転数
	int n = 0;

	int operate_buf[1000] = { 0 };

	const int dpx = toX(dest.psq());
	const int dcx = toX(dest.csq());
	const int dpy = toY(dest.psq());
	const int dcy = toY(dest.csq());

	necX = dpx - 3;// 3:現在の場所
	necY = 12 - std::max(dpy, dcy);

	if (dcx == dpx)
	{
		if (dpy < dcy)
			necR = 0;
		else
			necR = 2;
	}
	else
	{
		if (dpx < dcx)
			necR = 1;
		else
			necR = 3;
	}

	if (necY < 0)
		necY = 0;

	// 右移動
	if (necX > 0)
	{
		for (int i = 0; i < necX; i++)
			operate_buf[i * 2] |= RIGHT;

		if (necR == 3)
		{
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			operate_buf[0] |= L_ROTATE;
			operate_buf[2] |= L_ROTATE;
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}

	}
	// 左移動
	else if (necX < 0)
	{
		for (int i = 0; i < abs(necX); i++)
			operate_buf[i * 2] |= LEFT;

		if (necR == 3)
		{
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			operate_buf[0] |= R_ROTATE;
			operate_buf[2] |= R_ROTATE;
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}
	}

	else// 3列目におく場合
	{
		if (necR == 3)
		{
			operate_buf[0] |= L_ROTATE;
		}
		else if (necR == 2)
		{
			// 左右が詰まっている場合は、2回転は1回転ですむ
			if (!f.isEmpty(toSquare(4, 12)) && !f.isEmpty(toSquare(2, 12)))
			{
				operate_buf[0] |= R_ROTATE;
			}
			else if (!f.isEmpty(toSquare(4, 12)) && f.isEmpty(toSquare(2, 12)))
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
			else if (!f.isEmpty(toSquare(2, 12)) && f.isEmpty(toSquare(4, 12)))
			{
				operate_buf[0] |= R_ROTATE;
				operate_buf[2] |= R_ROTATE;
			}
			else if (!f.isEmpty(toSquare(4, 10)) || !f.isEmpty(toSquare(2, 10)))
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
			else
			{
				operate_buf[0] |= L_ROTATE;
				operate_buf[2] |= L_ROTATE;
			}
		}
		else if (necR == 1)
		{
			operate_buf[0] |= R_ROTATE;
		}
	}

	for (int i = 5; i < 1000; i += 20)
	{
		operate_buf[i] |= DOWN;
	}
	clear();

	for (int i = 0; i < 1000; i++)
		queue_.push(operate_buf[i]);

}