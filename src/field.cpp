#include <iostream>
#include "field.h"
#include "debug.h"
#include "bitboard.h"
#include "chain.h"

// �n�b�V���̎�
namespace { const Hashseed hash1, hash2; }

// field_���puyo���Z�b�g����	
void Field::set(const Tumo &t)
{
	field_[t.psq()] = t.pColor();
	field_[t.csq()] = t.cColor();
}

// current��EMPTY���Z�b�g����
void Field::setEmpty(const Tumo &t) { field_[t.psq()] = field_[t.csq()] = EMPTY; }

// �ݒu��A�V�����c�����v�[��������炤�B
void Field::tumoReload()
{
	current_ = tumo_pool_[next_++];

	if (next_ >= 128)
		next_ = 0;
}

// ������
void Field::init()
{
	// 0�N���A�������Ȃ������o�����ޔ�
	Flag s_flag = flag_;
	GameAssets* s_assets = assets;
	const Tumo* s_puyo_pool = tumo_pool_;
	const uint8_t* s_ojama_pool = ojama_pool_;

	// �����o�����ׂ�0�N���A
	memset(this, 0, sizeof(Field));

	// ���ɖ߂��D
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

	// �t���O�̂��|��(PLAYER�̏��ȊO��0�ɂ���)
	flag_ &= PLAYER_INFO;

	// �Q�[���J�n�O�͂��̏�Ԃɂ���@
	setFlag(WAIT_OJAMA);
}

// �ʒu��n���A�ړ�������B
// ���������true,���s�����false
// ���s�����ꍇ�͉������Ȃ�
bool Field::putTumo(const Tumo &t)
{
	setEmpty(current());// ���݂̏ꏊ��0�ɂ���

	// �Ղ悪�����Ȃ�
	if (!isEmpty(t.psq()) || !isEmpty(t.csq()))
	{
		set(current());// ���ɖ߂�
		return false;
	}
	else // �u����̂Œu���B
		set(t);

	return true;
}

// ��莞�Ԃ��ƂɈ�~��Ă��鏈��
// ���̍ہA�����蔭�� flag_ | TIGIRI
// �����łȂ��A�ʒu�m�莞 flag_ | SET
void Field::dropTumo()
{
	Tumo t = current();

	t.down();// ��ʒu������

	if (!putTumo(t))// �����u���Ȃ��Ȃ�A�����ňʒu���m�肳����
	{
		// �����菈��
		if (isEmpty(t.psq())) // �����ʒu�m�肵�Ă��A���ɋ󂫂���������
		{
			const bool pivot_empty = isEmpty(t.psq());
			setFlag(TIGIRI);
			tigiri_ = current().psq();
			remain_ = current().csq();
			++upper_y_[current().cx()]; // ��ԏ�̍��W���o���Ă���
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
			// ������Ȃ�
			++upper_y_[current().px()];
			++upper_y_[current().cx()];

			// �q�Ղ�̕����͒��ׂȂ����Ƃɒ���
			// �A�����𒲂ׂĂ���̂ŁA�q�Ղ�̕�����A�������Ă��܂��ƘA���������������Ȃ�
			setConnect(current().psq(), current().rotate());
			setConnect(current().csq());

			setFlag(SET);
		}
	}
	else // �ݒu�������Ȃ̂ŁA�ʒu���������B
		current_ = t;
}

// �\����
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

		// NEXT�Ղ��\��
		DxLib::DrawRectGraph(nextx, nexty + P_SIZE * i, 0, P_SIZE * c, P_SIZE, P_SIZE, assets->tumo.handle(), TRUE, FALSE);

		// NEXTNEXT�Ղ��\��
		DxLib::DrawRectGraph(next2x, next2y + P_SIZE * i, srcx, P_SIZE * nc, P_SIZE - 17, P_SIZE, assets->tumo.handle(), TRUE, FALSE);
	}

	// "�S����"�ƕ\��
	if (flag(ALLCLEAR))
	{
		const int destx = flag(PLAYER1) ? 68 : 418;
		assets->zenkesi.setPosition(destx, 108);
		assets->zenkesi.draw();
	}

	// �t�B�[���h��̂Ղ��\��
	// ���쒆�̂Ղ�͕ʂɕ\������
	for (int x = 1; x <= F_WID; x++)// �ԕ�������
	{
		for (int y = 1; y < F_HEI; y++)// �ԕ��A�����Ȃ�����������
		{
			const int destx = (flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + P_SIZE * (x - 1);
			const int desty = (flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + P_SIZE * (12 - y);
			const Square sq = toSquare(x, y);

			// ������܂Ղ�摜�́A�摜�t�@�C����ł��Ȃ藣�ꂽ�ʒu�ɑ��݂��Ă���̂�
			// ojamapuyo�Ƃ���Graph�^�̃C���X�^���X�Ƃ��Ď����Ă���
			if (color(sq) == OJAMA)
			{
				assets->ojamapuyo.setPosition(destx, desty);
				assets->ojamapuyo.draw();
				continue;
			}

			// �A�����ł͂Ȃ��A����
			// �����Ȃ����A���쒆�̂Ղ�Ȃ�\�����Ȃ�
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

			// �Ղ悪�����钼�O�ɂ́A��������̉摜��\������
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

			// ���݂̃t�B�[���h�̏�Ԃ�`�悷��
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

		// ���ݑ��쒆�̂Ղ��\��
		Square pos[2];
		int px = current().px(), py = current().py(), cx = current().cx(), cy = current().cy();
		Square psq = current().psq(), csq = current().csq();

		// current�ɂՂ悪�Ȃ���ΐ�؂�Ă��܂������̂ƍl����
		if (isEmpty(psq))
			psq = tigiri_;
		else if (isEmpty(csq))
			csq = tigiri_;

		// y�������̂ق��̂Ղ��pos[0]�ɁA��̂ق��̂Ղ��pos[1]�Ɋi�[
		pos[0] = py < cy ? psq : csq;
		pos[1] = py < cy ? csq : psq;

		int dest_x[2];
		int dest_y[2];
		dest_x[0] = (flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + P_SIZE * (toX(pos[0]) - 1);
		dest_y[0] = (flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + P_SIZE * (12 - toY(pos[0]));
		dest_x[1] = (flag(PLAYER1) ? P1_F_BEGINX : P2_F_BEGINX) + P_SIZE * (toX(pos[1]) - 1);
		dest_y[1] = (flag(PLAYER1) ? P1_F_BEGINY : P2_F_BEGINY) + P_SIZE * (12 - toY(pos[1]));

		// dropTumo()���Ă΂ꂽ�Ƃ��̏���
		// ������̃I�t�Z�b�g�͌�Ōv�Z����
		// �A�����͂Ƃ肠�����I�t�Z�b�g�Ȃ�
		// ��]�����c�Ȃ珬�����ق��̍��W�̉��ɉ����Ȃ�
		// ��]�������Ȃ炻�ꂼ��̂Ղ�̉��ɂՂ悪�Ȃ�
		if (!flag(TIGIRI) && !flag(RENSA) &&
			((!current().isSide() && isEmpty(pos[0] + SQ_DOWN)) ||
			(  current().isSide() && isEmpty(psq + SQ_DOWN) && isEmpty(csq + SQ_DOWN))))
		{
			// ���������ɂ��炵�ĕ\�����邽�߂̃I�t�Z�b�g�𑫂�
			dest_y[0] += y_offset;
			dest_y[1] += y_offset;
		}

		// �\������
		for (int i = 0; i < 2; i++)
		{
			// 14�i�ڂ͕\�����Ȃ�
			// 13�i�ڂ͕\������\���͂���                                                                                   
			if (toY(pos[i]) == RANK_14)
				continue;

			const Color c = color(pos[i]);

			// WAIT_NEXT����current�ɂՂ悪���݂��Ă��Ȃ�
			// �܂����ז����U�����Ƃ��Acurrent�����ז��ɂȂ邱�Ƃ�����
			if (c < RED || c == OJAMA)
				continue;

			const ColorType ct = colorToColorType(c);
			int srcx = P_SIZE * con_color[connect(pos[i])];
			int srcy = P_SIZE * ct;

			// ����͉��ړ��̂��߂̃I�t�Z�b�g
			dest_x[i] += x_offset;

			if (pos[i] == tigiri_)
				dest_y[i] += y_offset;

			// 13�i�ڂɂ���Ȃ�A���Ɍ����镔�������������炵�ĕ\��
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
			// �Ղ��ƒe�ޕ\��
			// �A�����ł͂Ȃ�
			// �����肪�������Ă��Ȃ�
			// �����肪�������Ă��Ă�����d�����ł���A�\������Ղ悪������Ďc�����ق��̂Ղ�ł���
			// �����蔭����̐ݒu�d���ł���A�����ꂽ�ق��̂Ղ�ł���
			else if (
				!flag(RENSA) &&
				flag(WAIT_TIGIRISET | WAIT_SET) &&
				(tigiri_ == SQ_ZERO || // �l���ݒ肳��Ă��Ȃ��Ƃ���SQ_ZERO�ɂȂ�d�l
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

	// �X�R�A�\��
	// �`����x���W�̓X�R�A�̌����Ɉˑ�
	// ���������߂�
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

	// �A������\��
	// �A�����N������ԉ��̈�ԉE�̍��W ��chain_right_bottom_
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

		// �A�����N��������̌��̋ʂ�\������
		float start_radian = pai_3_7;
		float dx, dy;

		if (flag(PLAYER1))
			dx = ojama_ ? 127.0f : 478.0f;
		else
			dx = ojama_ ? 478.0f : 127.0f;

		dy = 17;

		// koudan_x = dx + sin(3��/7) * n���
		float n = (koudan_x - dx) / sin(start_radian);
		koudan_x = dx + n * sin(start_radian + (pai - start_radian) * (1 - sin(pai_2 + pai_2 * percent)));
		float y = dy - koudan_y;// y�̕K�v�Ȉړ���
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

	// ���ז��Ղ�̕\��
	// �{���̒ʃ��[��
	// ���Ղ�i1���j
	// ��Ղ�i6���j
	// ��Ղ�i30���j
	// �L�m�R�Ղ�i200���j
	// ���Ղ�i300���j
	// �����Ղ�i400���j
	// �U�邨�ז��Ղ悪���� == �\���Ղ��\�����Ȃ��Ă͂����Ȃ�
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
		// �����𔲂������_�ŁA�\���Ղ�̎�ނ����ꂼ�ꉽ�\�������ׂ�����ojama[i]�ɓ����Ă���

		// i�͕\���������\���Ղ�̍ő�l
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

// ���Ƃ��Ղ悪�����flag_ | SLIDE�@�Ȃ����flag(~SLIDE
// SLIDE�ł��Ȃ�������false��Ԃ�
bool Field::slide()
{
	bool ret = false;
	clearFlag(SLIDE);

	for (int x = 1; x <= 6; x++)
	{
		for (int y = 1; y <= RANK_14; y++)
		{
			if (isEmpty(toSquare(x, y))) // �󔒂���������
			{
				// ���ɂ����𒸓_���W�Ƃ��Ă����B
				upper_y_[x] = Rank(y);

				// ���̏�����̂Ղ��T��
				do {
					++y;
				} while (y <= RANK_14 && isEmpty(toSquare(x, y)));

				if (y > RANK_14)// ������ɉ����Ȃ����
					break;
				
				setFlag(SLIDE);
				ret = true;

				do {
					if (!isEmpty(toSquare(x, y)))
					{
						// �������Ղ���A�󔒂̏ꏊ�Ɉڂ�
						field_[toSquare(x, y) + SQ_DOWN] = field_[toSquare(x, y)];
						field_[toSquare(x, y)] = EMPTY;

						// �Ō��swap�����ꏊ���ŏ�ʂ̂Ղ�
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
	// ���łɏ����Ȃ����Ƃ��킩���Ă���ꏊ���L�����Ă������߂�bitboard
	Bitboard bb_allready(0, 0), bb_over4(0, 0);

	ChainElement ce(chain());

	// �������F�ɉ�����bit��1�ɂ��Ă����āC���popcount�Ő�����D
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

		// recul_y_�͘A�����N��������ԁ���y���W�������Ă���̂ŁAwy-1�ł͂Ȃ�wy�ł����΂����悤�ȋC�����邪�A
		// ���ۂ�wy�ł���2�q�����Ă���ꏊ���������̂ŁAwy-1�Ŏg�p���Ă���
		for (int y = wy - 1; y < upper(x); y++)
		{
			const Square sq = toSquare(x, y);
			const Connect c = connect(sq);

			// �A����������Ƃ��́A�Œ�ł��ЂƂ͓�ȏ�Ȃ����Ă���Ղ悪���݂���͂��D
			if(bb_allready.isNotSet(sq) && (c & (c - 1)))
			{
				// �Ȃ����Ă����
				Bitboard bb = bbSameColor(sq);
				bb_allready |= bb;
				const int count = bb.count();
				
				if (count >= 4)
				{
					bb_over4 |= bb;
					color_bit |= 1 << colorToColorType(color(sq));

					// �A���{�[�i�X���v�Z
					ce.connectPlus(count);
				}
			}
		}
	}

	const bool is_ren = bb_over4.isTrue();

	if(is_ren)
	{
		// ���������F���{�[�i�X
		ce.setColorBonus(Bitop::popCount(color_bit));

		// �����J�E���g
		ce.setPuyoNum(bb_over4.count());

		// ������ʒu��bitboard�͂��ƂŎg���̂ŃR�s�[�n������
		deletePuyo<true>(Bitboard(bb_over4));

		setFlag(RENSA | SLIDE);
		
		// connect�������łɃ{�[�i�X�v�Z�ς�
		int bonus = ce.chainScore();

		// 1�A���Ȃ�{�[�i�X�_0�̘A�������肦��D
		if (bonus == 0)
			bonus = 1;

		// �S����
		if (flag(ALLCLEAR))
		{
			score_ += RATE * 30;
			clearFlag(ALLCLEAR);
		}
		
		chain_++;
		score_ += bonus;
		score_sum_ += bonus;

		// �A�����N������ԉE���̍��W���L�����Ă���
		// �A�����N�����ꏊ��dammy��2�ɂȂ��Ă���Ƃ���B

		// ��ԍ���ɂ��Ă���
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

// �ڒn�������ATIGIRI���͂����ASET���Z�b�g
// TIGIRI���O�ꂽ��false���߂�ifall���s�I�ȈӖ��Łj
bool Field::fall()
{
	field_[tigiri_ + SQ_DOWN] = field_[tigiri_];
	field_[tigiri_] = EMPTY;
	--tigiri_;

	if (!isEmpty(tigiri_ + SQ_DOWN))
	{
		// ��������ȏタ���ꂽ�Ղ悪�����ł��Ȃ��̂ŁA�ʒu���m�肳����
		++upper_y_[toX(tigiri_)];
		setConnect(tigiri_);
		clearFlag(TIGIRI);// �ݒu����
		setFlag(SET);
		return false;
	}
	return true;
}

void Field::ojamaFall(Field &enemy)
{
	int rand_ojama[6] = { 1, 2, 3, 4, 5, 6 };

	// ���肪�A������A�����̂��ז����U���Ă���Œ��͂�����܂Ղ�͐U��Ȃ�
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
				ojama_ -= 30;// ���̊֐����Ăяo���ꂽ�͂��߂̈��̎��_�ł̂��ז��Ղ���~�点��
				enemy.score_max_ -= RATE * 30;
			}
			else
			{
				ojama_buf_ = ojama_;
				ojama_ = 0;
				enemy.score_max_ -= RATE * ojama_buf_;
			}
			setFlag(OJAMA_FALLING);// ������܂Ղ旎�����t���O
		}

		if (ojama_buf_ >= 6)
		{
			ojama_buf_ -= 6;

			for (int x = 1; x <= 6; x++)
				field_[toSquare(x, 14)] = OJAMA;
		}
		else// ������܂Ղ悪�U�����̂Ƃ�
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
			assert(ojama_buf_ == 0);// 0�ɂȂ��Ă���͂�
			setFlag(OJAMAOK);
		}
	}
}

// �d�����ԂƁA������܂Ղ�A�Q�[���I�[�o�[����
int Field::wait(Field &enemy)
{
	if (flag(WAIT)) // �d������
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
			if (!ojama_)// ������܂Ղ悪�c���Ă��Ȃ�
				clearFlag(OJAMA_WILLFALL);

			if (flag(OJAMA_RESERVE))// ���ז��Ղ悪�ӂ��Ă���Œ��ɑ���̘A�����I������ꍇ
			{
				assert(ojama_);// �����ɂ���Ƃ������Ƃ͂�����܂Ղ�͎c���Ă���͂�
				clearFlag(OJAMA_RESERVE);
				setFlag(OJAMA_WILLFALL);
			}
			clearFlag(WAIT_OJAMA | OJAMAOK | OJAMA_FALLING);

			if (isDeath())// �A�����I����Ă��A[3][12]�ɂՂ悪����
				return -1;

			// 14����߂̂Ղ����������
			for (int x = 1; x <= 6; x++)
				field_[toSquare(x, 14)] = EMPTY;

			// ��������͂��Ă������[�h�̎�t�^�C�~���O�́A����ZURU_OK�����s���ꂽ�^�C�~���O�ł���D
			setFlag(ZURU_OK);
			return 1;
		}
		else if (flag(ZURU_OK))
		{
			clearFlag(ZURU_OK);

			// AI�͂���WAIT_NEXT�����s���ꂽ�Ƃ��Ɉ�񂾂��v�l����B
			// �������Ԃ��������Ă����ł͂Ȃ���Ƃ͂����ł���Ă��܂��B
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
			r_count_ = l_count_ = r_rotate_count_ = l_rotate_count_ = down_count_ = 0;// AI�p�̎d�l�i�Y���C�j
			tigiri_= Square(0);
			remain_ = Square(0);

			// �Ȃ�Putpuyo�𕪂��邩�Ƃ����ƁAAI���Ăяo���ۂ�(3,12)�ɂՂ悪�u����Ă���ƕs�s���Ȃ���
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

						// �A�����i�h�[���Ƃ����炫��Ƃ��j
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

			// wait_�����ɖ߂�
			// ���R�����̃I�t�Z�b�g�l�̐���������邽�߁B
			wait_ = fall_w_;
		}
		else
			assert(false);
	}
	return 0;
}

// game�̏���
bool Field::procedure(Field &enemy, Operate *ope)
{
	int rc = wait(enemy);

	if (rc == 1){ ; }// �������Ȃ�
	else if (rc == -1)
	{
		// �Q�[���I��
		return false;
	}
	else if (flag(TIGIRI))// �����菈��
	{
		wait_ = 0;
		setFlag(WAIT_TIGIRI);

		if (!fall())// 1�������Ƃ�
		{
			if (!flag(STATIC_MAKING))
				assets->drop.play();

			setFlag(WAIT_SET);
		}
	}
	else if (flag(SET))// �ݒu�������āA�e�ރG�t�F�N�g�����s������ɂ����ɗ���
	{
		wait_ = 0;

		if (flag(SLIDE))// �A����A�Ղ旎����
		{
			setFlag(WAIT_SLIDE);

			if (!slide())// �X���C�h����
			{
				setConnectRecul();
				clearFlag(WAIT_SLIDE);
				setFlag(WAIT_SET);

				// ������Ƃ��̂Ղ�Ɉ�����Ă���
				deleteMark();
			}
		}
		else
		{
			if (flag(RENSA))// �A���r��
			{
				if (vanish())// �A�����N�����Ƃ�
				{
					// 6�A���ڂ܂ł͕��ʂ̘A���{�C�X�B
					// 7�A���ȍ~�͓����A���{�C�X���g���B
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
					setFlag(WAIT_RENSA); // �A����̍d������	
					chain_stage_ = wait_;

					offseting(enemy);// ������܂Ղ悪�\�񂳂�Ă���Ȃ�A���E����
				}
				else// �A���I��
				{
					// bonusInit();
					chain_max_ = 0;

					// �������肪������܂Ղ旎�����Ȃ�
					if (enemy.flag(OJAMA_FALLING))
					{
						if (enemy.ojama_)
							enemy.setFlag(OJAMA_RESERVE);// ������ܗ������I�������~��t���O
					}
					else
					{
						// ������܂Ղ悪�~���Ă���Ԃ́A�V���Ȃ�����܂Ղ�͑���Ȃ�
						// ����ɂ�����܂��U��Ȃ��ꍇ��ojama_�t���O�͗��ĂĂĂ͂����Ȃ��i���Ă�ƁA���ז����U��܂Ńt���O�������Ȃ�����j
						if (enemy.ojama_)
							enemy.setFlag(OJAMA_WILLFALL);// �����̘A�����I�������A����Ɏ��ɔ�����܂Ղ悪�~��t���O�𗧂ĂĂ���
						else
							enemy.clearFlag(OJAMA_WILLFALL);
					}
					if (isEmpty())// �S����
					{
						setFlag(ALLCLEAR);

						if (!flag(STATIC_MAKING))
							assets->allclear.play();
					}
				}
			}
			else if (canDelete())// �A���ł��A�Ղ悪�����Ă���Œ��ł��Ȃ�
			{
				if (!flag(REPLAY_MODE))
				{
					// ���̏����̓Q�[���ɂ͊֌W�Ȃ��B��A���̃��O���c������
					Field f(*this);

					//Key w_key = f.key();
					f.keyInit();

					//assert(w_key == f.key());

					while (f.deleteMin());

					// AI�p�ɘA�����A�_�����L�����Ă���	
					// ���͂⏉��AI�͂ӂ邢�̂ł���Ȃ������B
					score_max_ += f.score();
					enemy.score_max_ -= f.score();
					chain_max_ = f.chain_max_;

					// rensa.txt�Ƃ����t�@�C���Ƀ��O���c���D
					if (chain_max_ > 11)
						simuraterConvert(f.score());
				}

				// ������Ƃ��̂Ղ�Ɉ�����Ă���
				deleteMark();

				setFlag(WAIT_SET);
				return true;
			}

			if (!flag(RENSA))// �A�����N���Ȃ������ꍇ
			{
				// �V�����Ղ���o��������
				setFlag(WAIT_OJAMA);
			}
		}
	}
	else// �A���̓r���ł͂Ȃ�
	{
		if (wait_ % FALLTIME == 0)
		{
			int p = processInput(ope);

			if (p & DOWN)// ���{�^���������ꂽ���i����ȊO�Ȃ牟���ꂽ�����Ɉړ��A��]������j
			{
				score_++;
				score_sum_++;

				// 2�Ƃ�������Ȃ�A���������炵�ĕ\�����鏈��������
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

			// TODO:��]���̕`�悪�r���D
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
			// ���{�^����������Ă��Ȃ����Ƃɂ�鎩�R�����ɂ��I�t�Z�b�g
			freedrop_offset_ += (float)P_SIZE / (float)FREEDROPTIME;
		}
	}
	wait_++;

	// ���݂̃t�B�[���h�̏�Ԃ��������ɕ`�悷��
	if (!flag(STATIC_MAKING))
		show();

	return true;
}

int Field::processInput(Operate *ope)
{
	assert(flag(PLAYER1 | PLAYER2) || ope);

	// �����ɉ������Ƃ�������Ȃ����������Aif-elseif�ɂ��Ă���
	int	operate_bit = 0;
	int ret = 0;
	Tumo n = current();
	bool b = true;

	// ����̎d�l
	// ��]�L�[���������ςȂ��ł���]���Ȃ��D
	// ���ړ��́A�E�A���ǂ��炩�̈ړ��̂ݎ󂯕t����
	// �t�B�[���h��ŉ�]�ł��Ȃ��ꏊ��14�i�ڂł̉�]�̂݁i15�i�ڂɕǂ����݂���j
	
	if (flag(REPLAY_MODE))
	{
		if (flag(PLAYER1))
			operate_bit = assets->operate_history_1p.pop();
		else
			operate_bit = assets->operate_history_2p.pop();
	}
	else
	{
		if (flag(PLAYER1))// 1p�p
		{
			if (flag(PLAYER_AI) || flag(ZURU_MODE))
				operate_bit = ope->pop();
			else
			{
					// �L�[���͂ɉ����ď���
					if (CheckHitKey(KEY_INPUT_LEFT))	operate_bit |= LEFT;
			   else if (CheckHitKey(KEY_INPUT_RIGHT))	operate_bit |= RIGHT;
					if (CheckHitKey(KEY_INPUT_X))		operate_bit |= R_ROTATE;
			   else	if (CheckHitKey(KEY_INPUT_Z))		operate_bit |= L_ROTATE;
			   		if (CheckHitKey(KEY_INPUT_DOWN))	operate_bit |= DOWN;
			}
		}
		else if (flag(PLAYER2))// 2p�p
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

		// ����̗���������Ă����D�i���v���C�Đ��̂��߁j
		if (flag(PLAYER1))
			assets->operate_history_1p.push(operate_bit);
		else
			assets->operate_history_2p.push(operate_bit);
	}
	if (operate_bit & LEFT)// ���������������ꂽ��
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
	else if (operate_bit & RIGHT)// ���������������ꂽ��
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

	if (operate_bit & R_ROTATE)// �E��]
	{
		if (r_rotate_count_ == 0)
		{
			n.rightRotate();

			if (!putTumo(n))
			{
				// ��]�������Ղ悪�������ł��遁���݂̂Ղ悪�c����
				if (n.isSide())
				{
					// ��]�ł��Ȃ������獶�E�ǂ��炩���ǂɂȂ��Ă���B
					// �Ղ悪�E�����irotate == 1)�Ȃ獶�Ɉړ��A������(rotate == 3)�Ȃ�E�Ɉړ�
					n.addx(File(n.rotate() - 2));

					if (!putTumo(n))
					{
						// ����ł��u���Ȃ�������A���x�͍��E�ǂ���Ƃ��ǂɂȂ��Ă���̂ŁA2��]������
						n.addx(-File(n.rotate() - 2));

						// �E�オ�J���Ă���
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

							if (!putTumo(n))// �����܂ł��Ēu���Ȃ�������A15�i�ڂ̕ǂɂԂ����Ă���̂ő��삵�Ȃ�
								b = false;
						}
					}
				}

				else// ������
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
					if (!putTumo(n))// �����܂ł��Ēu���Ȃ�������A15�i�ڂ̕ǂɂԂ����Ă���̂ő��삵�Ȃ�
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
	else if (operate_bit & L_ROTATE)// ����]
	{
		if (l_rotate_count_ == 0)
		{
			n.leftRotate();

			if (!putTumo(n))// �u���Ȃ��Ȃ�
			{
				if (n.isSide())// ���݂̂Ղ悪�c����
				{
					// ��]�ł��Ȃ������獶�E�ǂ��炩���ǂɂȂ��Ă���
					n.addx(File(n.rotate() - 2));

					if (!putTumo(n))
					{
						// ����ł��u���Ȃ�������A���x�͍��E���ǂɂȂ��Ă���̂ŁA2��]������
						n.addx(-File(n.rotate() - 2));

						if (isEmpty(n.psq() + SQ_LEFT + SQ_UP) && n.rotate() == 3)
						{
							// ���オ�J���Ă���
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

	if (operate_bit & DOWN)// ���������������ꂽ��
	{
		ret |= DOWN;// ���Ɉړ�����̂ł͂Ȃ��A��莞�Ԃ��Ƃɉ��ɉ���Ă��������iBlockDown)���ĂԂ��߂�true��Ԃ��悤�ɂ��Ă���
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

// �Â��ȋǖʂɂȂ�܂ŁA�ǖʂ�i�߂�(WAIT_NEXT�����s�����܂�)
// ��������ł���ǖʂ������ꍇ��-1��Ԃ�
// count��+�A��������-�̏ꍇ��FALLTIME�̔{���ɂȂ�̂Ŏ���ł���ǖʂ̏ꍇ�ȊO��-1�ɂȂ邱�Ƃ͂Ȃ��B
int Field::generateStaticState(Field& enemy)
{
	int count = 0;

	// ���삪�K�v�łȂ��ǖʂ܂Ői��ł����ꍇ
	if (flag(SET | TIGIRI | RENSA | SLIDE | WAIT_OJAMA | ZURU_OK))
	{
		// STATIC_MAKING���Z�b�g������Ԃ�procedure���Ăяo���Ή����ʕ\���A���v���C���M���O���s��Ȃ�
		setFlag(STATIC_MAKING);

		// WAIT_NEXT�����s�����܂ŃQ�[����i�߂�
		while (!flag(WAIT_NEXT))
		{
			count++;

			if (!procedure(enemy, NULL))
				return -1;
		}
		clearFlag(STATIC_MAKING);
		bonusInit();
		assert(examine());
	}
	else
	{
		// �܂����쒆��������Ȃ��̂ŁA���쒆�̂Ղ������
		setEmpty(current());

		// �T�Z�ŗ����܂łɂ����鎞�Ԃ��Z�o���Ă���
		// 12�i�ڂ���̋���*1�}�X���Ƃ��̂ɂ����鎞�Ԃ������̎c�莞�Ԃ�������Ă���
		// �Ȃ��Ȃ瑊��͂��̕����������������I��邩��B
		count -= (12 - std::min(current().py(), current().cy())) * FALLTIME;

		// ������K�v�Ȃ���������Ȃ����O�̂���
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
					// x,y�͂�����Ղ�ł���Ƃ������ƂŃ}�[�N������
					// VANISH�͍ė��p����
					delete_mark_[sq] = color(sq);
					delete_mark_[sq] |= VANISH;
				}
			}
		}
	}
}

// �ʔ������ȘA���Ȃ�Ղ�V�~�����[�^�œǂݍ��߂�`��URL�𐶐�����
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

	// 0 �� 1 �� 2 �� 3 �� 4 �� 5 �� 6 ���ז�
	bool b = false;

	for (int y = 13; y >= 1; y--)
	{
		for (int x = 1; x <= 6; x++)// ���ォ�珇�Ԃ�
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
