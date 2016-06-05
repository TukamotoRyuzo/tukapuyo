#pragma once

#include "sound.h"
#include "graph.h"
#include "fps.h"
#include "log.h"
#include "puyo.h"
#include "const.h"
#include <string>

enum GameMode { TOKOTON, HUTARIDE, AITO, GAMEMODE_MAX };
void operator ++ (GameMode &id);
void operator -- (GameMode &id);

struct GameAssets
{
	// メインメニューで使用
	Sound bgm_menu, click, cursor;
	Graph menu, button[GAMEMODE_MAX], button_t[GAMEMODE_MAX];

	// メニュー画面で使う画像や音の初期化
	void menuInit();
	void menuFinal();

	// ゲームで使用
	Graph field, border, tumo, choose, ojamapuyo, yatta, batankyuu, zenkesi,
		score_all, score_all2, chain_all, yokoku[6], score_1p[10], score_2p[10], 
		chain_num[10], chain_str, chain_str_2p, normal_tumo[5], vanish_face[5], 
		vanish_chip[5][9], lightball_1p, lightball_2p, sousai, star, win_star,
		win_num[10], you_win, you_lose;

	Sound bgm_battle, bgm_arrange, drop, decide, move, rotate, allclear, win, lose, lose_voice,
		chain[7], voice_1p[7], voice_2p[7], chain_effect[4];

	// 128ツモ
	Tumo tumo_pool[TUMO_MAX];

	// 対戦の記録用。対戦の記録、再生には以下の情報が必要。
	// 1) 1P、2Pの操作の履歴
	// 2) 試合で使われた128ぷよのすべて
	// 3) おじゃまぷよの降らすために使われたrandom関数の出力値

	// 1P,2Pの操作の履歴
	History operate_history_1p, operate_history_2p;

	// randomの履歴
	History ojama_history;
	std::string player_name;

	uint8_t ojama_rand_[128];

	void gameInit();
	void gameFinal();
};