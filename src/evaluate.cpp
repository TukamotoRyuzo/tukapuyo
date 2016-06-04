#include "field.h"
#include "chain.h"

// 引数で与えたおじゃまぷよ数が単純4個消しの何連鎖に相当するかを返す．
inline int ojamaToChainNum(const Score ojama)
{
	// OJAMA_AMOUNT[chain]とするとchain連鎖目でいくつのお邪魔ぷよが発生するかわかる．
	static const int OJAMA_AMOUNT[20] =
	{
		0,
		0, 4, 13, 31, 67, 121, 194, 285, 394, 522, 668, 832, 1014, 1215, 1434, 1671, 1927, 2201, 2493
	};
	// OJAMA_AMOUNTの中から，iより大となる最初の要素へのイテレータを返す
	const auto it = std::upper_bound(std::begin(OJAMA_AMOUNT), std::end(OJAMA_AMOUNT), ojama);
	const ptrdiff_t i = it - OJAMA_AMOUNT;
	return static_cast<int>(i - 1);
}

// 引数のお邪魔ぷよを発生させるのに必要なぷよ量を求める．
inline Score ojamaToPuyoNum(const Score ojama) { return static_cast<Score>(ojamaToChainNum(ojama) * 4); }

Score LightField::positionBonus() const
{
	Score score = SCORE_ZERO;

	// 平均高さを取得
	const int height_ave = (upper(1) + upper(2) + upper(3) + upper(4) + upper(5) + upper(6)) / 6;

	for (int x = 1; x <= 6; x++)
	{
		if (height_ave > 2)
		{
			int diff = abs(height_ave - upper(x));

			// 平均の高さより3以上の差があるならマイナス
			if (diff > 3)
			{
				score -= static_cast<Score>((diff - 3) * (diff - 3));
			}

			// 谷を低評価
			// 右隣と左隣との差が両方とも3以上であれば差の大きいほうの２乗誤差をペナルティとする
			if (upper(x) < upper(x - 1) - 2 && upper(x) < upper(x + 1) - 2)
			{
				const int diff_min = std::max(upper(x - 1) - upper(x), upper(x + 1) - upper(x));

				// 端にできる谷ほど低評価
				score -= static_cast<Score>(abs(3 - x) * diff_min * diff_min);
			}

			// 山を低評価
			if (upper(x) > upper(x - 1) + 2 && upper(x) > upper(x + 1) + 2)
			{
				const int diff_max = std::max(upper(x) - upper(x - 1), upper(x) - upper(x + 1));

				// 真ん中にできる山ほど低評価
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

// 時間と最大連鎖量、ツモ量から伸ばせる点数を大体で計算する。
inline Score chainMargin(const int remain_time, const int puyo_num, const int ojama_num, int chain_score)
{
	// 現在の点数を 点数 * (現在のぷよの数 + おけるぷよの数) / (現在のぷよの数)にできると仮定

	// 現局面から引けるツモの数
	const int can_put_num = (remain_time / ONEPUT_TIME) * 2;

	// それらをおいたときのすべてのぷよの数（おじゃまぷよは除く）
	const int field_num = (puyo_num + can_put_num) < 78 - ojama_num ? puyo_num + can_put_num : 78 - ojama_num;

	// フィールドのぷよが現在0個なら1個と考える
	const int p_num = (puyo_num ? puyo_num : 1);

	// 現在連鎖がまったくないならお邪魔10個の連鎖があると考える
	if (chain_score < 10)
		chain_score = 10;

	// 伸ばすことができる点数
	const Score margin = Score(int((float)chain_score * ((float)field_num / (float)p_num) - chain_score));

	return margin;
}

// 自分の最大連鎖と与えられた時間から、スコアを計算
// self_scoreは自分の持っていると思われる最大連鎖のスコアでお邪魔いくつ分という値
// これは、自分の次のツモを無視してフィールドのみから探索された連鎖である。
// なので、実際に打てる連鎖かどうかはわからない。２手以内に打てる連鎖はchainsList()に入っているので本当に打てる最大連鎖はmax_elementで取得するべき。
// self_scoreを評価に使ってよいのはremain_timeが３手以上置ける余裕があるときだと思われる。(remain_time >= 3 * ONEPUT_TIME)
// self_scoreはもし連鎖が今すぐ打てない形だったら小さな値になる．また，おじゃまぷよが12個以上または致死量振りそうなときは
// self_scoreは現在から1手，もしくは2手以内に打てる連鎖の点数になっている．
Score LightField::timeAndChainsToScore(const Score self_score, const Score enemy_score, const int remain_time, const LightField& enemy) const
{
	// 自分に振りそうなお邪魔ぷよの数
	const Score ojama = static_cast<Score>(ojama_);

	// 自分が何手か置ける余裕がある．
	if (remain_time >= 0)
	{
		// おじゃまぷよが振りそう
		if (ojama)
		{
			// 対応できるかどうか
			bool respond = false;

			// INT_MAX : 連鎖時間をありえない値で初期化
			Chains best_chain(0, INT_MAX, 0);

			// 今ある連鎖の中から，相手の連鎖より早く打ち終わって，なおかつ相手に致死量のお邪魔を遅れる連鎖を探す．
			// ついでに対応になっている連鎖を探す．
			for (auto chain = chainsList()->begin(); chain != chainsList()->end(); ++chain)
			{
				// 着手時間 + 連鎖時間を考慮しても時間が余っているなら勝ち．
				if (remain_time - (chain->time + chain->hakka_frame) >= 0 && chain->ojama - ojama >= enemy.deadLine())
				{
					return SCORE_KNOWN_WIN + chain->ojama - ojama;
				}

				// 赤玉一個を超えないくらいの対応ができる。				
				if (chain->ojama - ojama > 0 && chain->ojama - ojama <= 60 && (chain->hakka_frame < 45 ? true : (remain_time - chain->hakka_frame) > 0))
				{
					// TODO: 連鎖が対応と呼べるかどうか？
					// 自分のフィールドのぷよを半分以上使うようでは対応とはいえないかもしれない．
					respond = true;

					// 連鎖時間が短いほど的確な対応ということにする．
					if(chain->time < best_chain.time)
						best_chain = *chain;
				}
			}

			// 致死量振る場合
			if (ojama >= deadLine())
			{
				// 自分の現在の最大連鎖を打って返しても致死量振る．
				if (ojama - self_score > deadLine())
				{
					// あと2手しか置く余裕がなく発火するしかない状況であるなら、負け．
					// 2手以内に打てる最大の連鎖のスコアがself_scoreなので，これは正しい．
					// maxChainsScore()がそのような仕様になっている．
					if (remain_time <= ONEPUT_TIME * 3)
					{
						return SCORE_KNOWN_LOSE + self_score - ojama;
					}
					else // まだ3手以上の余裕がある。その間に伸ばせる点数を計算する。
					{
						// 挽回しなければならない点数
						const Score reverse = ojama - self_score;

						// 挽回することができる点数
						const Score margin = chainMargin(remain_time, field_puyo_num_, field_ojama_num_, self_score);

						// chainMarginで計算する伸びはかなり小さめに見積もっているので，marginがこのif文に入らない場合なら
						// 挽回できる可能性がある．
						if (reverse - (margin * 2) > deadLine())
						{
							// reverseが残り手数でどう考えても挽回できないなら、負け。
							// 伸びを考慮して、さらに点数を倍しても致死量振るなら、到底返せる連鎖ではない。
							// しかし，キーぷよを3つ以上おかなければ発火できない連鎖は発見できないので，self_scoreが間違っている可能性も考えられる．
							// ただ2手以内に打てる連鎖で今打たれている連鎖に対抗する手段はないということがわかっただけなので，
							// 本当はもう少し詳しくフィールドを調べなければならないのかもしれない．
							return SCORE_KNOWN_LOSE + margin;
						}
						else // 伸ばせば返せるかもしれない．
						{
							// 相手が相手のフィールドのぷよを半分以上使う連鎖をしているなら本線であると判断する．
							if (ojamaToPuyoNum(ojama) - enemy.field_puyo_num_ > 0)
							{
								// self_score個のお邪魔ぷよを発生させる連鎖は単純4個消しの何連鎖に相当するか？
								const int chain = ojamaToChainNum(self_score);

								// marginは相手の連鎖が終わる直前まで伸ばしたと仮定したときの量なので、打った連鎖時間がそのまま相手の残り時間になる。
								// enemyはすでに連鎖を打っているので、連鎖を打った後の形まで進んでいるはず
								const Score enemy_margin = chainMargin(CHAINTIME * chain, enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);
								return self_score + margin - ojama - enemy_score - enemy_margin;
							}
							else
							{
								// 相手は本線ではなく、大きめの催促を打っている．しかしここにくるということは相手がフィールドの
								// 半分以下のぷよの消費で自分に致死量の連鎖を降らせ、
								// なおかつ自分はその連鎖を現時点で本線と思われる連鎖を打っても返すことはできないということになる。
								// 考えられるパターンは
								// 1. 自分のフィールドのぷよ量が非常に少ないが全部伸ばせば何とか返せるかも知れない。
								// 2. ぷよ量は十分にあるが、キーぷよ３個以上置かなければ発火できない。
								// 3. 自分の３段目が高く積まれていて少ないお邪魔ぷよでもすぐ死にそうな状態で、２連鎖以上を打たれた。しかし自分には今すぐにそれを返せる連鎖はない。
								// である。1は負けと判定できるかもしれない。2と3は危機だが、何とかできる可能性はかなりある。
								// まだ3手以上の余裕があるので、なんとかできるかを考える。

								// 1なら負けとする
								if (field_puyo_num_ < enemy.field_puyo_num_)
								{
									return SCORE_KNOWN_LOSE + self_score + margin - ojama - enemy_score;
								}
								else if (remain_time < ONEPUT_TIME * 3) // 3手しか置けない
								{
									// 2手以内に打てない連鎖を3手で打てる形にもっていけるだろうか？
									// 必要色と必要ツモ数が得れる可能性はあまり高くないと思われる．
									// 真のスコアは探索しなければわからないが，こういう危なっかしい局面は避けるべき．
									return SCORE_KNOWN_LOSE + self_score;
								}
								else if (ojama < 30) // 少量のお邪魔が致死量になるのなら3段目を高く積んでいると思われる
								{
									// 4手以上置ける。									
									// とりあえず、伸ばせば返せそうだし4手以上引けるならその連鎖を打てるだろうと仮定。
									// しかし相手は的確に攻撃してきているといえる。不利といえるかもしれない。
									// ペナルティを与える．
									return self_score + margin - ojama - enemy_score - (SCORE_KNOWN_LOSE / 500);
								}
								else
								{
									// 4手以上引けて、相手に本線ではない大きめの催促を打たれいるが、今ある本線ではない連鎖を伸ばせば返せそう。
									// とりあえずよくわからんが不利ではないだろう。
									return self_score + margin - ojama - enemy_score;
								}
							}
						}
					}
				}
				else // 自分に振りそうなお邪魔ぷよは，現在の最大連鎖で返せる量である。
				{
					if (ojamaToPuyoNum(ojama) < enemy.field_puyo_num_) // 相手が本線を打っていない．ということは的確に攻撃されている．
					{
						// 相手が本線を打っておらず，かつ致死量のお邪魔が振りそう．
						
						// 対応できるならその点数を返す．
						if(respond)
						{
							return self_score + best_chain.ojama - ojama - enemy_score;
						}
						else // 対応できない．
						{
							// 時間がない．ということはself_score個分の本線を打つしかないが，相手に伸ばされるだろう．
							if (remain_time < ONEPUT_TIME * 3)
							{
								// 2手以内に打てる最大の連鎖
								auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
									[](Chains& a, Chains& b) { return a.ojama < b.ojama; });

								// 時間がなければself_score == chain_maxになるはず．
								assert(self_score < 4 || self_score == chain_max->ojama);

								// その間に相手に伸ばされる量
								const Score enemy_margin = chainMargin(chain_max->time - remain_time + chain_max->hakka_frame, 
									enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

								// 相手よりぷよ量が少ない，または現時点での相手の本線より小さい本線を打たざるを得ない状況ならば
								// ほとんど勝ち目はないだろう．
								if(field_puyo_num_ < enemy.field_puyo_num_ || self_score < enemy_score)
								{									
									return (SCORE_KNOWN_LOSE / 10) + self_score - ojama - enemy_score - enemy_margin;
								}								
								else if(ojama < 30)
								{
									// 少量のお邪魔が致死量になっているということは，的確な攻撃をされているということである．
									// 対応できないなら本線を打つしかないのでペナルティを与える．
									return (SCORE_KNOWN_LOSE / 100) + self_score - ojama - enemy_score - enemy_margin;
								}
								else
								{
									// 対応できないなら本線を打つしかないのでペナルティを与える．
									// 相手は催促としては大きめの連鎖を打ってくれているので，そこまで不利ではないかも知れない．
									return self_score - ojama - enemy_score - enemy_margin;
								}
							}
							else // 3手以上おく余裕がある．しかし対応できない．
							{
								// TODO:

								// 相手よりぷよ量が少ない，または現時点での相手の本線より自分の本線が小さい
								if(field_puyo_num_ < enemy.field_puyo_num_ || self_score - ojama < enemy_score)
								{									
									return (SCORE_KNOWN_LOSE / ((remain_time / 100) + 1)) + self_score - ojama - enemy_score;
								}								
								else if(ojama < 30)
								{
									// 少量のお邪魔が致死量になっているということは，的確な攻撃をされているということである．
									// 対応できないなら本線を打つしかないのでペナルティを与える．
									return (SCORE_KNOWN_LOSE / ((remain_time / 10) + 1)) + self_score - ojama - enemy_score;
								}
								else
								{
									// 対応できないなら本線を打つしかないのでペナルティを与える．
									// 相手は催促としては大きめの連鎖を打ってくれているので，そこまで不利ではないかも知れない．
									return (SCORE_KNOWN_LOSE / (remain_time + 1)) + self_score - ojama - enemy_score;
								}
							}
						}
					}
					else // 相手が本線を打っているが，自分の本線で返せそう．
					{
						// しかし、もしかしたら時間がなければ発火できない可能性がある。
						if (remain_time < ONEPUT_TIME * 3)
						{
							// 2手以内に打てる最大の連鎖
							auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
								[](Chains& a, Chains& b) { return a.ojama < b.ojama; });

							// 時間がなければself_score == chain_maxになるはず．
							assert(self_score < 4 || self_score == chain_max->ojama);

							// 2手以内に打てる連鎖の中で致死量を返しきる連鎖がない。または返せる量だとしても時間内に打つことができない。
							if (ojama - chain_max->ojama >= deadLine() || chain_max->hakka_frame > remain_time)
							{
								// でもここくることはないはず…
								return SCORE_KNOWN_LOSE + chain_max->ojama - ojama;
							}
							else // どうやら本当に返せる。
							{
								// しかし打っても有利になるとは限らない。打ったとしたら相手はセカンドを当然作るから．
								// 自分が本線を打ったと仮定したときの相手が伸ばせる量
								const Score enemy_margin = chainMargin(chain_max->time - remain_time + chain_max->hakka_frame,
									enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

								if (chain_max->ojama - ojama - enemy_score - enemy_margin >= enemy.deadLine())
								{
									// 相手がセカンドで返せないような連鎖を打っている。
									return (SCORE_KNOWN_WIN / 1000) + chain_max->ojama - ojama - enemy_score - enemy_margin;
								}
								return chain_max->ojama - ojama - enemy_score - enemy_margin;
							}
						}
						else // 3手以上置く余裕がある．ここは↑よりも更に有利であると思われる．
						{
							// 自分が伸ばせる量
							const Score margin = chainMargin(remain_time, field_puyo_num_, field_ojama_num_, self_score);

							const int time = ojamaToChainNum(self_score) * CHAINTIME;

							// 相手が伸ばせる量
							const Score enemy_margin = chainMargin(time, enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

							return self_score + margin - ojama - enemy_margin;
						}
					}
				}
			}
			else // 致死量振らない場合。この時は催促や全消しを取られたとき
			{
				if (respond) // 対応できるので，それなりの点数を返す．
				{
					return self_score + best_chain.ojama - ojama - enemy_score;
				}
				else // 対応できない
				{
					// 時間がない．
					if (remain_time < ONEPUT_TIME * 3)
					{
						// 2手以内に打てる最大の連鎖
						auto chain_max = std::max_element(chainsList()->begin(), chainsList()->end(),
							[](Chains& a, Chains& b) { return a.ojama < b.ojama; });

						// 相手がどれくらい伸ばせるか
						const Score enemy_margin = chainMargin(chain_max->time + chain_max->hakka_frame - remain_time,
							enemy.field_puyo_num_, enemy.field_ojama_num_, enemy_score);

						// 本線を打っていいのだろうか？
						if(ojama > 18)
						{
							// 3列以上振ることを許容できる場合はほとんどない．
							// 対応もできないので本線を打つしかない．					
							return chain_max->ojama - ojama - enemy_score - enemy_margin;
						}
						else // 3列以下なら許容してもいいかもしれない．
						{
							if(ojama < 6)
							{
								return self_score - ojama - enemy_score;
							}

							// 仮にお邪魔ぷよを降らせて見る．
							LightField f(*this);
							int o = ojama;
							f.ojamaFallMin(&o);

							// 致死量振るかもしれない。deadLine()は確実に死ぬ量を返すので、たとえば11段目までつんでいたら
							// 1個振るだけで死ぬ場合もある。
							if (f.isDeath())
							{
								// 本線を打つしかない。
								if (field_puyo_num_ < enemy.field_puyo_num_ || self_score < enemy_score)
								{
									// ぷよ量で負けているか本線で負けているならかなり悪い．
									return (SCORE_KNOWN_LOSE / 100) + self_score - enemy_score - ojama;
								}
								else // ぷよ量では勝っている．探索しなければわからない．
								{
									return self_score - enemy_score - ojama;
								}
							}
							// 色補完をして連鎖があるかどうか調べる．時間のかかる処理だが，ここは正確性を優先する．
							else if (f.colorHelper(2) < 50 * RATE)
							{
								Chains e_best_chain(0, INT_MAX, 0);

								// 連鎖が埋まっている.
								// このとき，相手に追い討ちがあるかどうか調べる．
								// 相手が2手以内に打てる連鎖の中で致死量を送ることができて，なおかつ一番短い連鎖を探す．
								for(auto enemy_chain = enemy.chainsList()->begin(); enemy_chain != enemy.chainsList()->end(); ++enemy_chain)
								{
									if(enemy_chain->ojama > f.deadLine())
									{
										if(enemy_chain->time < e_best_chain.time)
										{
											// 時間が短ければ短いほどよい連鎖
											e_best_chain = *enemy_chain;
										}
									}
								}

								// 短い追い討ちがある．
								if(e_best_chain.time < ONEPUT_TIME * 6)
								{
									// しかもこれを食らうと死ぬ．対応もできないので本線を打つしかない．						
									return chain_max->ojama - ojama - enemy_score - enemy_margin;
								}
								else // すぐの追い討ちはない．
								{
									// その間にもしかしたらお邪魔をほれるかもしれない
									if(field_puyo_num_ < enemy.field_puyo_num_ || self_score < enemy_score)
									{
										// ぷよ量で負けているか本線で負けているならかなり悪い．
										return (SCORE_KNOWN_LOSE / 100) + self_score - enemy_score - ojama;
									}
									else // ぷよ量では勝っている．ほれるなら勝ちだが，探索しなければわからない．
									{
										return self_score - enemy_score - ojama;
									}
								}
							}
							else
							{
								// つぶされはしないので食らっても問題はない．
								return self_score - ojama - enemy_score;
							}
						}
					}
					else // 3手以上おく余裕がある．しかし対応できない．
					{
						// TODO:

						// たぶん脅威ではない。
						if (ojama <= 12)
						{
							return self_score - ojama - enemy_score;
						}
						else if (ojama <= 24)
						{
							// 3～4列のお邪魔が振る。残り時間が十分にあるか、相手のツモが非常に少なければ脅威ではない。
							// 振った後の形が悪くなければ脅威ではないかも知れない。
							// ペナルティは与えるが、残り時間に応じてかなり小さくなる。相手の形が悪かったりツモが少なかったりしたら
							// enemy_scoreは小さな値になる。
							return (SCORE_KNOWN_LOSE / ((remain_time * 3) + 1)) + self_score - ojama - enemy_score;
						}
						else // 4列以上振る。
						{
							// 相手よりぷよ量が少ない，または現時点での相手の本線より自分の本線が小さい
							if (field_puyo_num_ < enemy.field_puyo_num_ || self_score - ojama < enemy_score)
							{
								return (SCORE_KNOWN_LOSE / ((remain_time / 2) + 1)) + self_score - ojama - enemy_score;
							}
							else if (ojama < 30)
							{
								// 少量のお邪魔が致死量になっているということは，的確な攻撃をされているということである．
								// 対応できないなら本線を打つしかないのでペナルティを与える．
								return (SCORE_KNOWN_LOSE / (remain_time + 1)) + self_score - ojama - enemy_score;
							}
							else
							{
								// 対応できないなら本線を打つしかないのでペナルティを与える．
								// 相手は催促としては大きめの連鎖を打ってくれているので，そこまで不利ではないかも知れない．
								return (SCORE_KNOWN_LOSE / (remain_time * 2+ 1)) + self_score - ojama - enemy_score;
							}
						}
					}
				}
			}
		}
		else // お邪魔が振らない.
		{
			return self_score + Score(remain_time / 12) - enemy_score;
		}
	}
	else // 時間なし。
	{
		// おじゃまぷよが振ることが確定
		if (ojama)
		{
			// 致死量振る場合
			if (ojama >= deadLine())
			{
				return SCORE_KNOWN_LOSE;
			}
			else
			{
				// 仮に降らせて見る
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

// 評価関数
// maxChainsScoreがconst関数ではない（chainsListを更新)ためconst関数ではないが、動作的にはconst関数である。
Score LightField::evaluate(LightField& enemy, int remain_time)
{
	assert(examine());
	assert(enemy.examine());

	// 死んでいる局面なら負け
	if (isDeath())
		return SCORE_LOSE;

	if (enemy.isDeath())
		return SCORE_WIN;

	// フィールドが持っている連鎖を求めておく．戻り値は最大連鎖のスコア.
	// 求めた連鎖は，this->chainsList()に入っている．
	Score self_score = this->maxChainsScore(2, remain_time) / RATE;
	Score enemy_score = enemy.maxChainsScore(2, -remain_time) / RATE;

	// 自分の持っている連鎖と与えられた時間からスコアを計算
	Score score = timeAndChainsToScore(self_score, enemy_score, remain_time, enemy);
//	Score score = Score(enemy.ojama()) - Score(ojama());

	// 連結数を評価
	score += connectBonus(-3, 1, 3) - enemy.connectBonus(-3, 1, 3);

	// 位置評価
	score += positionBonus() - enemy.positionBonus();

	// 全消しを持っているなら、お邪魔30個分のボーナスを与える
	// 連鎖を持っているなら、全消しボーナスが加算されてあるので大丈夫
	if (flag(ALLCLEAR) && self_score == SCORE_ZERO)
		score += static_cast<Score>(30);

	if (enemy.flag(ALLCLEAR) && enemy_score == SCORE_ZERO)
		score -= static_cast<Score>(30);

	return score;
}