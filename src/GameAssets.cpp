#include "GameAssets.h"
#include "const.h"

void operator ++ (GameMode &id) { id = static_cast<GameMode>(id + 1); }
void operator -- (GameMode &id) { id = static_cast<GameMode>(id - 1); }

void GameAssets::menuInit()
{
	// サウンドの用意
	bgm_menu.load(".\\bgm\\BGM_menu.wav");

	// 決定音
	click.load(".\\effect\\Tsu\\decide.ogg");
	cursor.load(".\\effect\\Tsu\\cursor.ogg");

	// 画像の用意
	menu.load(".\\png\\menu.png", TRUE);
	menu.setPosition(0, 0);

	button[0].load(".\\png\\tokoton.png", TRUE);
	button[1].load(".\\png\\hutaride.png", TRUE);
	button[2].load(".\\png\\aito.png", TRUE);
	button_t[0].load(".\\png\\tokoton2.png", TRUE);
	button_t[1].load(".\\png\\hutaride2.png", TRUE);
	button_t[2].load(".\\png\\aito2.png", TRUE);

	button[0].setPosition(220, 270);
	button[1].setPosition(220, 320);
	button[2].setPosition(220, 370);
	button_t[0].setPosition(220, 270);
	button_t[1].setPosition(220, 320);	
	button_t[2].setPosition(220, 370);

}

void GameAssets::menuFinal()
{
	DxLib::InitGraph();
	DxLib::DeleteSoundMem(bgm_menu.handle());
}

void GameAssets::gameInit()
{
	// 背景画像
	field.load(".\\png\\Tsu Background 10\\field.png",false);
	field.setPosition(0, 0);

	// ぷよ画像
	tumo.load(".\\png\\puyo.png", true);
	//tumo.load(".\\png\\Tsu.png", true);

	// お邪魔ぷよ画像を作る
	ojamapuyo.init(DerivationGraph(192, 384, P_SIZE, P_SIZE, tumo.handle()), true);     
	yokoku[0].init(DerivationGraph(448, 384, P_SIZE, P_SIZE, tumo.handle()), true);// tibi
	yokoku[1].init(DerivationGraph(416, 384, P_SIZE, P_SIZE, tumo.handle()), true);// deka
	yokoku[2].init(DerivationGraph(384, 384, P_SIZE, P_SIZE, tumo.handle()), true);// iwa
	yokoku[3].init(DerivationGraph(384, 352, P_SIZE, P_SIZE, tumo.handle()), true);// kinoko
	yokoku[4].init(DerivationGraph(352, 352, P_SIZE, P_SIZE, tumo.handle()), true);// hosi
	yokoku[5].init(DerivationGraph(320, 352, P_SIZE, P_SIZE, tumo.handle()), true);// king

	// normalなぷよ画像を用意しておく
	for (int i = 0 ; i < 5; i++)
		normal_tumo[i].init(DxLib::DerivationGraph(0, P_SIZE * i, P_SIZE, P_SIZE, tumo.handle()), true);

	// 全消し！
	zenkesi.load(".\\png\\Tsu Background 10\\allclear.png", true);

	// レベル選択枠
	border.load(".\\png\\level.png", true);
	border.setPosition(60, 141);

	// カレーのアイコン
	choose.load(".\\png\\choose.png", true);
	choose.setPosition(185, 240);

	// やった
	yatta.load(".\\png\\Tsu Background 10\\win.png", true);

	// ばたんきゅ～
	batankyuu.load(".\\png\\Tsu Background 10\\lose.png", true);

	// 音声ファイル
	cursor.load(".\\effect\\Tsu\\cursor.ogg");
	drop.load(".\\effect\\Tsu\\drop.ogg");
	decide.load(".\\effect\\Tsu\\decide.ogg");
	bgm_battle.load(".\\bgm\\Puyo Puyo 2\\Area A (MD).ogg");
	win.load(".\\voice\\win.wav");
	lose.load(".\\effect\\Tsu\\lose.ogg");
	lose_voice.load(".\\voice\\lose.wav");

	// 連鎖時の音(2,3,4以降の音）
	chain_effect[0].load(".\\effect\\Tsu\\nuisance_hitS.ogg");
	chain_effect[1].load(".\\effect\\Tsu\\nuisance_hitM.ogg");
	chain_effect[2].load(".\\effect\\Tsu\\nuisance_hitL.ogg");
	chain_effect[3].load(".\\effect\\Tsu\\heavy.ogg");

	vanish_face[0].init(DxLib::DerivationGraph(0, P_SIZE * 12, P_SIZE, P_SIZE, tumo.handle()), true);
	vanish_face[1].init(DxLib::DerivationGraph(0, P_SIZE * 13, P_SIZE, P_SIZE, tumo.handle()), true);
	vanish_face[2].init(DxLib::DerivationGraph(P_SIZE * 2, P_SIZE * 12, P_SIZE, P_SIZE, tumo.handle()), true);
	vanish_face[3].init(DxLib::DerivationGraph(P_SIZE * 2, P_SIZE * 13, P_SIZE, P_SIZE, tumo.handle()), true);
	vanish_face[4].init(DxLib::DerivationGraph(P_SIZE * 4, P_SIZE * 12, P_SIZE, P_SIZE, tumo.handle()), true);

	// ".\\puyopuyo_erase
	const char *col[5] = {"red", "green", "blue", "yellow", "purple"};
	char buf[80];

	for (int color = 0; color < 5; color++)
	{
		for (int frame = 0; frame < 9; frame++)
		{
			sprintf(buf, ".\\puyopuyo_erase\\puyo_%s_erase\\puyo_%s_erase_0%d.png", col[color], col[color], frame+1);
			vanish_chip[color][frame].load(buf, true);
		}
	}

	for (int i = 0; i < 7; i++)
	{
		char buf[80];
		sprintf(buf, ".\\effect\\Tsu\\chain%d.ogg", i + 1);
		chain[i].load(buf);
		sprintf(buf, ".\\voice\\chain%d_1p.wav", i + 1);
		voice_1p[i].load(buf);
		voice_1p[i].changeVolume(255);
		sprintf(buf, ".\\voice\\chain%d_2p.wav", i + 1);
		voice_2p[i].load(buf);
	}
	move.load(".\\effect\\Tsu\\move.ogg");
	rotate.load(".\\effect\\Tsu\\rotate.ogg");
	allclear.load(".\\effect\\Tsu\\allclear.ogg");

	// 点数の画像
	score_all.load(".\\png\\Tsu Background 10\\score.png", true);
	score_all2.load(".\\png\\Tsu Background 10\\score2p.png", true);
	chain_all.load(".\\png\\Tsu Background 10\\chain.png", true);
	for (int i = 0; i < 10; i++)
	{
		score_1p[i].init(DerivationGraph(3 + 20 * i, 3, 15, 33, score_all.handle()), true);
		score_2p[i].init(DerivationGraph(3 + 20 * i, 3, 15, 33, score_all2.handle()), true);
		chain_num[i].init(DerivationGraph(20 * i, 10, 15, 28, chain_all.handle()), true);
	}
	chain_str.init(DerivationGraph(200, 10, 50, 28, chain_all.handle()), true);
	chain_str_2p.init(DerivationGraph(253, 10, 50, 28, chain_all.handle()), true);

	int handle = DxLib::LoadGraph(".\\png\\koudan.png", TRUE);
	lightball_1p.init(DerivationGraph(0, 0, P_SIZE, P_SIZE, handle), true);
	lightball_2p.init(DerivationGraph(P_SIZE, 0, P_SIZE, P_SIZE, handle), true);
	sousai.load(".\\png\\sousai.png", true);

	star.load(".\\carbuncle\\star.png", true);
	win_star.load(".\\carbuncle\\star2.png", true);
	int winnum_handle = DxLib::LoadGraph(".\\png\\Tsu Background 10\\fcounter.png", true);

	for (int i = 0; i < 10; i++)
	{
		win_num[i].init(DerivationGraph(20 * i, 0, 20, 27, winnum_handle), true);
	}

	you_win.load(".\\png\\youwin.png", true);
	you_win.setPosition(0, 0);
	you_lose.load(".\\png\\youlose.png", true);
	you_lose.setPosition(0, 0);
}

void GameAssets::gameFinal()
{
	DxLib::InitGraph();
	DxLib::InitSoundMem();
}
