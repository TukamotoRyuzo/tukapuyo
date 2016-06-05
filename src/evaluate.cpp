#include "field.h"
#include "chain.h"

// �����ŗ^����������܂Ղ搔���P��4�����̉��A���ɑ������邩��Ԃ��D
inline int ojamaToChainNum(const Score ojama)
{
	// OJAMA_AMOUNT[chain]�Ƃ����chain�A���ڂł����̂��ז��Ղ悪�������邩�킩��D
	static const int OJAMA_AMOUNT[20] =
	{
		0,
		0, 4, 13, 31, 67, 121, 194, 285, 394, 522, 668, 832, 1014, 1215, 1434, 1671, 1927, 2201, 2493
	};
	// OJAMA_AMOUNT�̒�����Ci����ƂȂ�ŏ��̗v�f�ւ̃C�e���[�^��Ԃ�
	const auto it = std::upper_bound(std::begin(OJAMA_AMOUNT), std::end(OJAMA_AMOUNT), ojama);
	const ptrdiff_t i = it - OJAMA_AMOUNT;
	return static_cast<int>(i - 1);
}

// �����̂��ז��Ղ�𔭐�������̂ɕK�v�ȂՂ�ʂ����߂�D
inline Score ojamaToPuyoNum(const Score ojama) { return static_cast<Score>(ojamaToChainNum(ojama) * 4); }

Score LightField::positionBonus() const
{
	Score score = SCORE_ZERO;

	// ���ύ������擾
	const int height_ave = (upper(1) + upper(2) + upper(3) + upper(4) + upper(5) + upper(6)) / 6;

	for (int x = 1; x <= 6; x++)
	{
		if (height_ave > 2)
		{
			int diff = abs(height_ave - upper(x));

			// ���ς̍������3�ȏ�̍�������Ȃ�}�C�i�X
			if (diff > 3)
			{
				score -= static_cast<Score>((diff - 3) * (diff - 3));
			}

			// �J���]��
			// �E�ׂƍ��ׂƂ̍��������Ƃ�3�ȏ�ł���΍��̑傫���ق��̂Q��덷���y�i���e�B�Ƃ���
			if (upper(x) < upper(x - 1) - 2 && upper(x) < upper(x + 1) - 2)
			{
				const int diff_min = std::max(upper(x - 1) - upper(x), upper(x + 1) - upper(x));

				// �[�ɂł���J�قǒ�]��
				score -= static_cast<Score>(abs(3 - x) * diff_min * diff_min);
			}

			// �R���]��
			if (upper(x) > upper(x - 1) + 2 && upper(x) > upper(x + 1) + 2)
			{
				const int diff_max = std::max(upper(x) - upper(x - 1), upper(x) - upper(x + 1));

				// �^�񒆂ɂł���R�قǒ�]��
				score -= static_cast<Score>(abs(abs(x - 3) - 3) * diff_max * diff_max);
			}
		}
		for (int y = 1; y < upper(x); y++)
		{
			const Square sq = toSquare(x, y);

			if (x > 1 && x < 6 && connect(sq) == (CON_UP | CON_DOWN))
				score -= Score(5);

			if (y == 1 && upper(x) > 2 && !connect(sq))
				score -= Score(3);

			if (x == 1)
			{
				if (connect(sq) == (CON_UP | CON_RIGHT) || connect(sq) == (CON_DOWN | CON_RIGHT))
					score += Score(3);
			}
			else if (x == 2 || x == 5)
			{
				if (connect(sq) == (CON_RIGHT | CON_LEFT))
					score += Score(3);
			}
			else if (x == 6)
			{
				if (connect(sq) == (CON_UP | CON_LEFT) || connect(sq) == (CON_DOWN | CON_LEFT))
					score += Score(3);
			}
		}
	}
	/*if(connect(A1) == CON_RIGHT && connect(B1) == CON_LEFT)
	{
	score += static_cast<Score>(5);

	if (connect(B2) == CON_LEFT)
	{
	score += static_cast<Score>(5);

	if ((Connect)puyo(A2) & CON_UP)
	{
	score += static_cast<Score>(5);

	if (color(B1) == color(C2) && !connect(C2))
	{
	score += static_cast<Score>(5);

	if (color(C2) == color(B3) && !connect(B3))
	{
	score += static_cast<Score>(5);

	if (color(C1) == color(C3))
	{
	score += static_cast<Score>(5);

	if (color(C3) == color(D2))
	{
	score += static_cast<Score>(5);

	if (connect(D2) == CON_RIGHT || connect(D2) == CON_DOWN || connect(C3) == CON_UP || connect(D3) == (CON_LEFT | CON_DOWN))
	{
	score += static_cast<Score>(5);
	}
	}

	if (connect(B4) == (CON_RIGHT | CON_LEFT))
	{
	score += static_cast<Score>(5);
	}
	if (color(A3) == color(B4))
	{
	score -= static_cast<Score>(5);
	}
	}
	else
	score -= static_cast<Score>(3);
	}
	}
	}
	}
	}*/
	return score + static_cast<Score>(position_bonus_);
}

// ���Ԃƍő�A���ʁA�c���ʂ���L�΂���_�����̂Ōv�Z����B
inline Score chainMargin(const int remain_time, const int puyo_num, const int ojama_num, int chain_score)
{
	// ���݂̓_���� �_�� * (���݂̂Ղ�̐� + ������Ղ�̐�) / (���݂̂Ղ�̐�)�ɂł���Ɖ���

	// ���ǖʂ��������c���̐�
	const int can_put_num = (remain_time / ONEPUT_TIME) * 2;

	// �������������Ƃ��̂��ׂĂ̂Ղ�̐��i������܂Ղ�͏����j
	const int field_num = (puyo_num + can_put_num) < 78 - ojama_num ? puyo_num + can_put_num : 78 - ojama_num;

	// �t�B�[���h�̂Ղ悪����0�Ȃ�1�ƍl����
	const int p_num = (puyo_num ? puyo_num : 1);

	// ���ݘA�����܂������Ȃ��Ȃ炨�ז�10�̘A��������ƍl����
	if (chain_score < 10)
		chain_score = 10;

	// �L�΂����Ƃ��ł���_��
	const Score margin = Score(int((float)chain_score * ((float)field_num / (float)p_num) - chain_score));

	return margin;
}

// �����̍ő�A���Ɨ^����ꂽ���Ԃ���A�X�R�A���v�Z
// self_score�͎����̎����Ă���Ǝv����ő�A���̃X�R�A�ł��ז��������Ƃ����l
// ����́A�����̎��̃c���𖳎����ăt�B�[���h�݂̂���T�����ꂽ�A���ł���B
// �Ȃ̂ŁA���ۂɑłĂ�A�����ǂ����͂킩��Ȃ��B�Q��ȓ��ɑłĂ�A����chainsList()�ɓ����Ă���̂Ŗ{���ɑłĂ�ő�A����max_element�Ŏ擾����ׂ��B
// self_score��]���Ɏg���Ă悢�̂�remain_time���R��ȏ�u����]�T������Ƃ����Ǝv����B(remain_time >= 3 * ONEPUT_TIME)
// self_score�͂����A�����������łĂȂ��`�������珬���Ȓl�ɂȂ�D�܂��C������܂Ղ悪12�ȏ�܂��͒v���ʐU�肻���ȂƂ���
// self_score�͌��݂���1��C��������2��ȓ��ɑłĂ�A���̓_���ɂȂ��Ă���D
Score LightField::timeAndChainsToScore(const Score self_score, const Score enemy_score, const int remain_time, const LightField& enemy) const
{
	// �����ɐU�肻���Ȃ��ז��Ղ�̐�
	const Score ojama = static_cast<Score>(ojama_);

	// ���������肩�u����]�T������D
	if (remain_time >= 0)
	{
		// ������܂Ղ悪�U�肻��
		if (ojama)
		{
			// �Ή��ł��邩�ǂ���
			bool respond = false;

			// INT_MAX : �A�����Ԃ����肦�Ȃ��l�ŏ�����
			Chains best_chain(0, INT_MAX, 0);

			// ������A���̒�����C����̘A����葁���ł��I����āC�Ȃ�������ɒv���ʂ̂��ז���x���A����T���D
			// ���łɑΉ��ɂȂ��Ă���A����T���D
			for (auto chain = chainsList()->begin(); chain != chainsList()->end(); ++chain)
			{
				// ���莞�� + �A�����Ԃ��l�����Ă����Ԃ��]���Ă���Ȃ珟���D
				if (remain_time - (chain->time + chain->hakka_frame) >= 0 && chain->ojama - ojama >= enemy.deadLine())
				{
					return SCORE_KNOWN_WIN + chain->ojama - ojama;
				}

				// �ԋʈ�𒴂��Ȃ����炢�̑Ή����ł���B				
				if (chain->ojama - ojama > 0 && chain->ojama - ojama <= 60 && (chain->hakka_frame < 45 ? true : (remain_time - chain->hakka_frame) > 0))
				{
					// TODO: �A�����Ή��ƌĂׂ邩�ǂ����H
					// �����̃t�B�[���h�̂Ղ�𔼕��ȏ�g���悤�ł͑Ή��Ƃ͂����Ȃ���������Ȃ��D
					respond = true;

					// �A�����Ԃ��Z���قǓI�m�ȑΉ��Ƃ������Ƃɂ���D
					if(chain->time < best_chain.time)
						best_chain = *chain;
				}
			}

			// �v���ʐU��ꍇ
			if (ojama >= deadLine())
			{
				// �����̌��݂̍ő�A����ł��ĕԂ��Ă��v���ʐU��D
				if (ojama - self_score > deadLine())
				{
					// ����2�肵���u���]�T���Ȃ����΂��邵���Ȃ��󋵂ł���Ȃ�A�����D
					// 2��ȓ��ɑłĂ�ő�̘A���̃X�R�A��self_score�Ȃ̂ŁC����͐������D
					// maxChainsScore()�����̂悤�Ȏd�l�ɂȂ��Ă���D
					if (remain_time <= ONEPUT_TIME * 3)
					{
						return SCORE_KNOWN_LOSE + self_score - ojama;
					}
					else // �܂�3��ȏ�̗]�T������B���̊ԂɐL�΂���_�����v�Z����B
					{
						// �҉񂵂Ȃ���΂Ȃ�Ȃ��_��
						const Score reverse = ojama - self_score;

						// �҉񂷂邱�Ƃ��ł���_��
						const Score margin = chainMargin(remain_time, field_puyo_num_, field_ojama_num_, self_score);

						// chainMargin�Ōv�Z����L�т͂��Ȃ菬���߂Ɍ��ς����Ă���̂ŁCmargin������if���ɓ���Ȃ��ꍇ�Ȃ�
						// �҉�ł���\��������D
						if (reverse - (margin * 2) > deadLine())
						{
							// reverse���c��萔�łǂ��l���Ă��҉�ł��Ȃ��Ȃ�A�����B
							// �L�т��l�����āA����ɓ_����{���Ă��v���ʐU��Ȃ�A����Ԃ���A���ł͂Ȃ��B
							// �������C�L�[�Ղ��3�ȏエ���Ȃ���Δ��΂ł��Ȃ��A���͔����ł��Ȃ��̂ŁCself_score���Ԉ���Ă���\�����l������D
							// ����2��ȓ��ɑłĂ�A���ō��ł���Ă���A���ɑ΍R�����i�͂Ȃ��Ƃ������Ƃ��킩���������Ȃ̂ŁC
							// �{���͂��������ڂ����t�B�[���h�𒲂ׂȂ���΂Ȃ�Ȃ��̂�������Ȃ��D
							return SCORE_KNOWN_LOSE + margin;
						}
						else // �L�΂��ΕԂ��邩������Ȃ��D
						{
							// ���肪����̃t�B�[���h�̂Ղ�𔼕��ȏ�g���A�������Ă���Ȃ�{���ł���Ɣ��f����D
							if (ojamaToPuyoNum(ojama) - enemy.field_puyo_num_ > 0)
							{
								// self_score�̂��ז��Ղ�𔭐�������A���͒P��4�����̉��A���ɑ������邩�H
								const int chain = ojamaToChainNum(self_score);

								// margin�͑���̘A�����I��钼�O�܂ŐL�΂����Ɖ��肵���Ƃ��̗ʂȂ̂ŁA�ł����A�����Ԃ����̂܂ܑ���̎c�莞�ԂɂȂ�B
								// enemy�͂��łɘA����ł��Ă���̂ŁA�A����ł�����̌`�܂Ői��ł���͂�
								const Score enemy_margin = chainMargin(CHAINTIME * chain, enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);
								return self_score + margin - ojama - enemy_score - enemy_margin;
							}
							else
							{
								// ����͖{���ł͂Ȃ��A�傫�߂̍Ñ���ł��Ă���D�����������ɂ���Ƃ������Ƃ͑��肪�t�B�[���h��
								// �����ȉ��̂Ղ�̏���Ŏ����ɒv���ʂ̘A�����~�点�A
								// �Ȃ��������͂��̘A���������_�Ŗ{���Ǝv����A����ł��Ă��Ԃ����Ƃ͂ł��Ȃ��Ƃ������ƂɂȂ�B
								// �l������p�^�[����
								// 1. �����̃t�B�[���h�̂Ղ�ʂ����ɏ��Ȃ����S���L�΂��Ή��Ƃ��Ԃ��邩���m��Ȃ��B
								// 2. �Ղ�ʂ͏\���ɂ��邪�A�L�[�Ղ�R�ȏ�u���Ȃ���Δ��΂ł��Ȃ��B
								// 3. �����̂R�i�ڂ������ς܂�Ă��ď��Ȃ����ז��Ղ�ł��������ɂ����ȏ�ԂŁA�Q�A���ȏ��ł��ꂽ�B�����������ɂ͍������ɂ����Ԃ���A���͂Ȃ��B
								// �ł���B1�͕����Ɣ���ł��邩������Ȃ��B2��3�͊�@�����A���Ƃ��ł���\���͂��Ȃ肠��B
								// �܂�3��ȏ�̗]�T������̂ŁA�Ȃ�Ƃ��ł��邩���l����B

								// 1�Ȃ畉���Ƃ���
								if (field_puyo_num_ < enemy.field_puyo_num_)
								{
									return SCORE_KNOWN_LOSE + self_score + margin - ojama - enemy_score;
								}
								else if (remain_time < ONEPUT_TIME * 3) // 3�肵���u���Ȃ�
								{
									// 2��ȓ��ɑłĂȂ��A����3��őłĂ�`�ɂ����Ă����邾�낤���H
									// �K�v�F�ƕK�v�c�����������\���͂��܂荂���Ȃ��Ǝv����D
									// �^�̃X�R�A�͒T�����Ȃ���΂킩��Ȃ����C����������Ȃ��������ǖʂ͔�����ׂ��D
									return SCORE_KNOWN_LOSE + self_score;
								}
								else if (ojama < 30) // ���ʂ̂��ז����v���ʂɂȂ�̂Ȃ�3�i�ڂ������ς�ł���Ǝv����
								{
									// 4��ȏ�u����B									
									// �Ƃ肠�����A�L�΂��ΕԂ���������4��ȏ������Ȃ炻�̘A����łĂ邾�낤�Ɖ���B
									// ����������͓I�m�ɍU�����Ă��Ă���Ƃ�����B�s���Ƃ����邩������Ȃ��B
									// �y�i���e�B��^����D
									return self_score + margin - ojama - enemy_score - (SCORE_KNOWN_LOSE / 500);
								}
								else
								{
									// 4��ȏ�����āA����ɖ{���ł͂Ȃ��傫�߂̍Ñ���ł��ꂢ�邪�A������{���ł͂Ȃ��A����L�΂��ΕԂ������B
									// �Ƃ肠�����悭�킩��񂪕s���ł͂Ȃ����낤�B
									return self_score + margin - ojama - enemy_score;
								}
							}
						}
					}
				}
				else // �����ɐU�肻���Ȃ��ז��Ղ�́C���݂̍ő�A���ŕԂ���ʂł���B
				{
					if (ojamaToPuyoNum(ojama) < enemy.field_puyo_num_) // ���肪�{����ł��Ă��Ȃ��D�Ƃ������Ƃ͓I�m�ɍU������Ă���D
					{
						// ���肪�{����ł��Ă��炸�C���v���ʂ̂��ז����U�肻���D
						
						// �Ή��ł���Ȃ炻�̓_����Ԃ��D
						if(respond)
						{
							return self_score + best_chain.ojama - ojama - enemy_score;
						}
						else // �Ή��ł��Ȃ��D
						{
							// ���Ԃ��Ȃ��D�Ƃ������Ƃ�self_score���̖{����ł����Ȃ����C����ɐL�΂���邾�낤�D
							if (remain_time < ONEPUT_TIME * 3)
							{
								// 2��ȓ��ɑłĂ�ő�̘A��
								auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
									[](Chains& a, Chains& b) { return a.ojama < b.ojama; });

								// ���Ԃ��Ȃ����self_score == chain_max�ɂȂ�͂��D
								assert(self_score < 4 || self_score == chain_max->ojama);

								// ���̊Ԃɑ���ɐL�΂�����
								const Score enemy_margin = chainMargin(chain_max->time - remain_time + chain_max->hakka_frame, 
									enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

								// ������Ղ�ʂ����Ȃ��C�܂��͌����_�ł̑���̖{����菬�����{����ł�����𓾂Ȃ��󋵂Ȃ��
								// �قƂ�Ǐ����ڂ͂Ȃ����낤�D
								if(field_puyo_num_ < enemy.field_puyo_num_ || self_score < enemy_score)
								{									
									return (SCORE_KNOWN_LOSE / 10) + self_score - ojama - enemy_score - enemy_margin;
								}								
								else if(ojama < 30)
								{
									// ���ʂ̂��ז����v���ʂɂȂ��Ă���Ƃ������Ƃ́C�I�m�ȍU��������Ă���Ƃ������Ƃł���D
									// �Ή��ł��Ȃ��Ȃ�{����ł����Ȃ��̂Ńy�i���e�B��^����D
									return (SCORE_KNOWN_LOSE / 100) + self_score - ojama - enemy_score - enemy_margin;
								}
								else
								{
									// �Ή��ł��Ȃ��Ȃ�{����ł����Ȃ��̂Ńy�i���e�B��^����D
									// ����͍Ñ��Ƃ��Ă͑傫�߂̘A����ł��Ă���Ă���̂ŁC�����܂ŕs���ł͂Ȃ������m��Ȃ��D
									return self_score - ojama - enemy_score - enemy_margin;
								}
							}
							else // 3��ȏエ���]�T������D�������Ή��ł��Ȃ��D
							{
								// TODO:

								// ������Ղ�ʂ����Ȃ��C�܂��͌����_�ł̑���̖{����莩���̖{����������
								if(field_puyo_num_ < enemy.field_puyo_num_ || self_score - ojama < enemy_score)
								{									
									return (SCORE_KNOWN_LOSE / ((remain_time / 100) + 1)) + self_score - ojama - enemy_score;
								}								
								else if(ojama < 30)
								{
									// ���ʂ̂��ז����v���ʂɂȂ��Ă���Ƃ������Ƃ́C�I�m�ȍU��������Ă���Ƃ������Ƃł���D
									// �Ή��ł��Ȃ��Ȃ�{����ł����Ȃ��̂Ńy�i���e�B��^����D
									return (SCORE_KNOWN_LOSE / ((remain_time / 10) + 1)) + self_score - ojama - enemy_score;
								}
								else
								{
									// �Ή��ł��Ȃ��Ȃ�{����ł����Ȃ��̂Ńy�i���e�B��^����D
									// ����͍Ñ��Ƃ��Ă͑傫�߂̘A����ł��Ă���Ă���̂ŁC�����܂ŕs���ł͂Ȃ������m��Ȃ��D
									return (SCORE_KNOWN_LOSE / (remain_time + 1)) + self_score - ojama - enemy_score;
								}
							}
						}
					}
					else // ���肪�{����ł��Ă��邪�C�����̖{���ŕԂ������D
					{
						// �������A�����������玞�Ԃ��Ȃ���Δ��΂ł��Ȃ��\��������B
						if (remain_time < ONEPUT_TIME * 3)
						{
							// 2��ȓ��ɑłĂ�ő�̘A��
							auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
								[](Chains& a, Chains& b) { return a.ojama < b.ojama; });

							// ���Ԃ��Ȃ����self_score == chain_max�ɂȂ�͂��D
							assert(self_score < 4 || self_score == chain_max->ojama);

							// 2��ȓ��ɑłĂ�A���̒��Œv���ʂ�Ԃ�����A�����Ȃ��B�܂��͕Ԃ���ʂ��Ƃ��Ă����ԓ��ɑł��Ƃ��ł��Ȃ��B
							if (ojama - chain_max->ojama >= deadLine() || chain_max->hakka_frame > remain_time)
							{
								// �ł��������邱�Ƃ͂Ȃ��͂��c
								return SCORE_KNOWN_LOSE + chain_max->ojama - ojama;
							}
							else // �ǂ����{���ɕԂ���B
							{
								// �������ł��Ă��L���ɂȂ�Ƃ͌���Ȃ��B�ł����Ƃ����瑊��̓Z�J���h�𓖑R��邩��D
								// �������{����ł����Ɖ��肵���Ƃ��̑��肪�L�΂����
								const Score enemy_margin = chainMargin(chain_max->time - remain_time + chain_max->hakka_frame,
									enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

								if (chain_max->ojama - ojama - enemy_score - enemy_margin >= enemy.deadLine())
								{
									// ���肪�Z�J���h�ŕԂ��Ȃ��悤�ȘA����ł��Ă���B
									return (SCORE_KNOWN_WIN / 1000) + chain_max->ojama - ojama - enemy_score - enemy_margin;
								}
								return chain_max->ojama - ojama - enemy_score - enemy_margin;
							}
						}
						else // 3��ȏ�u���]�T������D�����́������X�ɗL���ł���Ǝv����D
						{
							// �������L�΂����
							const Score margin = chainMargin(remain_time, field_puyo_num_, field_ojama_num_, self_score);

							const int time = ojamaToChainNum(self_score) * CHAINTIME;

							// ���肪�L�΂����
							const Score enemy_margin = chainMargin(time, enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

							return self_score + margin - ojama - enemy_margin;
						}
					}
				}
			}
			else // �v���ʐU��Ȃ��ꍇ�B���̎��͍Ñ���S���������ꂽ�Ƃ�
			{
				if (respond) // �Ή��ł���̂ŁC����Ȃ�̓_����Ԃ��D
				{
					return self_score + best_chain.ojama - ojama - enemy_score;
				}
				else // �Ή��ł��Ȃ�
				{
					// ���Ԃ��Ȃ��D
					if (remain_time < ONEPUT_TIME * 3)
					{
						// 2��ȓ��ɑłĂ�ő�̘A��
						auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
							[](Chains& a, Chains& b) { return a.ojama < b.ojama; });

						// ���肪�ǂꂭ�炢�L�΂��邩
						const Score enemy_margin = chainMargin(chain_max->time + chain_max->hakka_frame - remain_time,
							enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

						// �{����ł��Ă����̂��낤���H
						if(ojama > 18)
						{
							// 3��ȏ�U�邱�Ƃ����e�ł���ꍇ�͂قƂ�ǂȂ��D
							// �Ή����ł��Ȃ��̂Ŗ{����ł����Ȃ��D					
							return chain_max->ojama - ojama - enemy_score - enemy_margin;
						}
						else // 3��ȉ��Ȃ狖�e���Ă�������������Ȃ��D
						{
							if(ojama < 6)
							{
								return self_score - ojama - enemy_score;
							}

							// ���ɂ��ז��Ղ���~�点�Č���D
							LightField f(*this);
							int o = ojama;
							f.ojamaFallMin(&o);

							// �v���ʐU�邩������Ȃ��BdeadLine()�͊m���Ɏ��ʗʂ�Ԃ��̂ŁA���Ƃ���11�i�ڂ܂ł�ł�����
							// 1�U�邾���Ŏ��ʏꍇ������B
							if (f.isDeath())
							{
								// �{����ł����Ȃ��B
								if (field_puyo_num_ < enemy.field_puyo_num_ || self_score < enemy_score)
								{
									// �Ղ�ʂŕ����Ă��邩�{���ŕ����Ă���Ȃ炩�Ȃ舫���D
									return (SCORE_KNOWN_LOSE / 100) + self_score - enemy_score - ojama;
								}
								else // �Ղ�ʂł͏����Ă���D�T�����Ȃ���΂킩��Ȃ��D
								{
									return self_score - enemy_score - ojama;
								}
							}
							// �F�⊮�����ĘA�������邩�ǂ������ׂ�D���Ԃ̂����鏈�������C�����͐��m����D�悷��D
							else if (f.colorHelper(2) < 50 * RATE)
							{
								Chains e_best_chain(0, INT_MAX, 0);

								// �A�������܂��Ă���.
								// ���̂Ƃ��C����ɒǂ����������邩�ǂ������ׂ�D
								// ���肪2��ȓ��ɑłĂ�A���̒��Œv���ʂ𑗂邱�Ƃ��ł��āC�Ȃ�����ԒZ���A����T���D
								for(auto enemy_chain = enemy.chainsList()->begin(); enemy_chain != enemy.chainsList()->end(); ++enemy_chain)
								{
									if(enemy_chain->ojama > f.deadLine())
									{
										if(enemy_chain->time < e_best_chain.time)
										{
											// ���Ԃ��Z����ΒZ���قǂ悢�A��
											e_best_chain = *enemy_chain;
										}
									}
								}

								// �Z���ǂ�����������D
								if(e_best_chain.time < ONEPUT_TIME * 6)
								{
									// �����������H�炤�Ǝ��ʁD�Ή����ł��Ȃ��̂Ŗ{����ł����Ȃ��D						
									return chain_max->ojama - ojama - enemy_score - enemy_margin;
								}
								else // �����̒ǂ������͂Ȃ��D
								{
									// ���̊Ԃɂ����������炨�ז����ق�邩������Ȃ�
									if(field_puyo_num_ < enemy.field_puyo_num_ || self_score < enemy_score)
									{
										// �Ղ�ʂŕ����Ă��邩�{���ŕ����Ă���Ȃ炩�Ȃ舫���D
										return (SCORE_KNOWN_LOSE / 100) + self_score - enemy_score - ojama;
									}
									else // �Ղ�ʂł͏����Ă���D�ق��Ȃ珟�������C�T�����Ȃ���΂킩��Ȃ��D
									{
										return self_score - enemy_score - ojama;
									}
								}
							}
							else
							{
								// �Ԃ���͂��Ȃ��̂ŐH����Ă����͂Ȃ��D
								return self_score - ojama - enemy_score;
							}
						}
					}
					else // 3��ȏエ���]�T������D�������Ή��ł��Ȃ��D
					{
						// TODO:

						// ���Ԃ񋺈Ђł͂Ȃ��B
						if (ojama <= 12)
						{
							return self_score - ojama - enemy_score;
						}
						else if (ojama <= 24)
						{
							// 3�`4��̂��ז����U��B�c�莞�Ԃ��\���ɂ��邩�A����̃c�������ɏ��Ȃ���΋��Ђł͂Ȃ��B
							// �U������̌`�������Ȃ���΋��Ђł͂Ȃ������m��Ȃ��B
							// �y�i���e�B�͗^���邪�A�c�莞�Ԃɉ����Ă��Ȃ菬�����Ȃ�B����̌`������������c�������Ȃ������肵����
							// enemy_score�͏����Ȓl�ɂȂ�B
							return (SCORE_KNOWN_LOSE / ((remain_time * 3) + 1)) + self_score - ojama - enemy_score;
						}
						else // 4��ȏ�U��B
						{
							// ������Ղ�ʂ����Ȃ��C�܂��͌����_�ł̑���̖{����莩���̖{����������
							if (field_puyo_num_ < enemy.field_puyo_num_ || self_score - ojama < enemy_score)
							{
								return (SCORE_KNOWN_LOSE / ((remain_time / 2) + 1)) + self_score - ojama - enemy_score;
							}
							else if (ojama < 30)
							{
								// ���ʂ̂��ז����v���ʂɂȂ��Ă���Ƃ������Ƃ́C�I�m�ȍU��������Ă���Ƃ������Ƃł���D
								// �Ή��ł��Ȃ��Ȃ�{����ł����Ȃ��̂Ńy�i���e�B��^����D
								return (SCORE_KNOWN_LOSE / (remain_time + 1)) + self_score - ojama - enemy_score;
							}
							else
							{
								// �Ή��ł��Ȃ��Ȃ�{����ł����Ȃ��̂Ńy�i���e�B��^����D
								// ����͍Ñ��Ƃ��Ă͑傫�߂̘A����ł��Ă���Ă���̂ŁC�����܂ŕs���ł͂Ȃ������m��Ȃ��D
								return (SCORE_KNOWN_LOSE / (remain_time * 2+ 1)) + self_score - ojama - enemy_score;
							}
						}
					}
				}
			}
		}
		else // ���ז����U��Ȃ�.
		{
			return self_score + Score(remain_time / 12) - enemy_score;
		}
	}
	else // ���ԂȂ��B
	{
		// ������܂Ղ悪�U�邱�Ƃ��m��
		if (ojama)
		{
			// �v���ʐU��ꍇ
			if (ojama >= deadLine())
			{
				return SCORE_KNOWN_LOSE;
			}
			else
			{
				// ���ɍ~�点�Č���
				LightField f(*this);
				int o = ojama_;
				f.ojamaFallMin(&o);
				return f.positionBonus();
			}
		}
		else
		{
			return self_score + Score(remain_time / 12) - enemy_score;
		}
	}
}

// �]���֐�
// maxChainsScore��const�֐��ł͂Ȃ��ichainsList���X�V)����const�֐��ł͂Ȃ����A����I�ɂ�const�֐��ł���B
Score LightField::evaluate(LightField& enemy, int remain_time)
{
	assert(examine());
	assert(enemy.examine());

	// ����ł���ǖʂȂ畉��
	if (isDeath())
		return -SCORE_INFINITE;

	if (enemy.isDeath())
		return SCORE_INFINITE;

	// �t�B�[���h�������Ă���A�������߂Ă����D�߂�l�͍ő�A���̃X�R�A.
	// ���߂��A���́Cthis->chainsList()�ɓ����Ă���D
	Score self_score = this->maxChainsScore(2, remain_time) / RATE;
	Score enemy_score = enemy.maxChainsScore(2, -remain_time) / RATE;

	// �����̎����Ă���A���Ɨ^����ꂽ���Ԃ���X�R�A���v�Z
	Score score = timeAndChainsToScore(self_score, enemy_score, remain_time, enemy);
//	Score score = Score(enemy.ojama()) - Score(ojama());

	// �A������]��
	score += connectBonus(-3, 1, 3) - enemy.connectBonus(-3, 1, 3);

	// �ʒu�]��
	score += positionBonus() - enemy.positionBonus();

	// �S�����������Ă���Ȃ�A���ז�30���̃{�[�i�X��^����
	// �A���������Ă���Ȃ�A�S�����{�[�i�X�����Z����Ă���̂ő��v
	if (flag(ALLCLEAR) && self_score == SCORE_ZERO)
		score += static_cast<Score>(30);

	if (enemy.flag(ALLCLEAR) && enemy_score == SCORE_ZERO)
		score -= static_cast<Score>(30);

	return score;
}