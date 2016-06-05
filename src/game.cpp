#include "gamemode.h"
#include "const.h"
#include "field.h"
#include "AI.h"
#include "debug.h"
#include "tt.h"
#include "GameAssets.h"
#include <algorithm>
#include <ctime>
#include <stack>
#include "chain.h"

//#define LEARN                                           
//#define LEARN2
//#define ZOROME
//#define AIVSAI
#define SPEED 10
#define AILEVEL_1p 4
#define ONE_PLAY_LINE 4

enum
{
	SEEDA = 1445, SEEDB = 3, SEEDC = 7, SEEDD = 349,
};

// ゲームの状態をここにコピーしておけば「待った」が可能になる。
struct GameState
{
	Field f1, f2;
	Operate ope_1p, ope_2p;
	GameState(const Field f_1, const Field f_2, const Operate op1, const Operate op2) : f1(f_1), f2(f_2), ope_1p(op1), ope_2p(op2) {};
};

typedef std::stack<GameState> GameStack;

// とことんと二人で、AIを受け持つ 
namespace Game
{
	enum Fase { FADEIN, BORDER_APPEAR, LEVEL_SELECT, PLAYING, BORDER_DISAPPEAR, RESULT };
	
	Fase fase;

	// 画像の構造体
	GameAssets* assets;

	// AI
	AI3Connect2 ai3con(AILEVEL_1p);
	AI3Connect2 ai3con2(3);
	PolytecAI poly_ai;

	// 1p, 2pのフィールド
	Field *f1, *f2;

	// 各Faseに対応して呼び出される
	void fadeIn();
	void borderAppear();
	void levelSelect();
	void playing(GameResult* result, const GameMode mode, int *p1_win, int* p2_win);
	void borderDisappear();
	void gameResult(const GameResult result, const int p1_win, const int p2_win);

	// Fase == PLAYINGの時に呼び出されるループ．
	GameResult playLoop(const GameMode mode);
	void initTumo();
	void ojamaRandInit();
	void gameOver();
	bool selectedReplay();
	bool selectedZuru();
	void saveGame();
	void putHelper(GameStack *gs);
	void reInit();
	void drawSummaryStar(const int p1_win, const int p2_win);

	bool isReplayMode() { return f1->flag(REPLAY_MODE) && f2->flag(REPLAY_MODE); }
	bool isZuruMode() { return f1->flag(ZURU_MODE) && f2->flag(ZURU_MODE); }

} // namespace Game

// ゲームに必要な画像，ツモの初期化，フィールドの初期化を行う．
void Game::init(GameAssets* s)
{
	assets = s;
	assets->gameInit();

	f1 = new Field(PLAYER1, assets);
	f2 = new Field(PLAYER2, assets);

	// 200MBの局面表を確保
	TT.resize(200);
	CT.setSize(200);
	
	// ツモの初期化とぷよぷよの内部情報の初期化
	reInit();
}

// メインループ．
int Game::loop(const GameMode mode)
{
	Fps fps;
	GameResult result = NO_RESULT;
	int p1_win = 0;
	int p2_win = 0;
	fase = FADEIN;

	// 2P側をAIにするか否か。
	if (mode == AITO)
		f2->setFlag(PLAYER_AI);
	else
		f2->clearFlag(PLAYER_AI);

	// bgm再生(さいしょのおんがく)
	assets->bgm_battle.play(DX_PLAYTYPE_LOOP);

	// メインループ
	while(!ScreenFlip() && !ProcessMessage() && !ClearDrawScreen())
	{
		// escキーが押されたらメニュー画面に戻る
		if (CheckHitKey(KEY_INPUT_E))
		{
			reInit();
			assets->bgm_battle.stop();
			assets->gameFinal();
			return 0;
		}

		switch(fase)
		{
			case FADEIN:			fadeIn(); break;
			case BORDER_APPEAR:		borderAppear(); break;
			case LEVEL_SELECT:		levelSelect(); break;
			case BORDER_DISAPPEAR:	borderDisappear(); break;
			case PLAYING:			playing(&result, mode, &p1_win, &p2_win); break;
			case RESULT:			gameResult(result, p1_win, p2_win); break;
			default: assert(false);
		}

		// 勝ち星を表示する。
		drawSummaryStar(p1_win, p2_win);

		fps.update();
		fps.wait();
	}

	return 0;
}

// プレイ中のループ．
GameResult Game::playLoop(const GameMode mode)
{
	GameResult result = NO_RESULT;
	static GameStack gs;

	assets->field.draw();

	if (mode == TOKOTON) // "とことん"用ループ
	{
		if(f1->flag(WAIT_NEXT))
		{
			ChainsList *s = new ChainsList;
			f1->setChains(s);
			LightField f(*f1);
			color_searched = allready_searched = chain_called = chain_hit = 0;
			HighPrecisionTimer tm;
			tm.begin();
			Score ss = f1->colorHelper(2);
			tm.end();
			MyOutputDebugString("持っている連鎖の点数 = %d, node = %d, かぶり = %d, elapsed = %.10lfs\n", ss, color_searched, allready_searched, tm.elapsed());
			MyOutputDebugString("chain_called = %d, chain_hit = %d\n", chain_called, chain_hit);
			assert(f == *f1);
			delete s;
		}
		if (!f1->procedure(*f2))
		{
			result = P2_WIN;
			assets->lose.play();
			assets->lose_voice.play();
		}
	}
	else if (mode == HUTARIDE) // "ふたりで"用ループ
	{
		if (f1->flag(WAIT_NEXT))
		{
			Field df1(*f1), df2(*f2);
			ChainsList *ss = new ChainsList;
			ChainsList *a = new ChainsList;
			df1.setChains(ss);
			df2.setChains(a);
			df1.chainsList()->push_back(Chains(0, 0, 0));
			df2.chainsList()->push_back(Chains(0, 0, 0));
			const int i = df2.generateStaticState(df1);
			HighPrecisionTimer tc;
			tc.begin();
			const Score s = df1.evaluate(df2, i);
			tc.end();
			MyOutputDebugString("score = %d\nelapsed = %.10lfs\n", s, tc.elapsed());
			delete ss;
			delete a;
		}

		if (!f1->procedure(*f2))
		{
			result = P2_WIN;
			assets->lose.play();
			assets->lose_voice.play();
		}

		if (!f2->procedure(*f1))
		{
			result = P1_WIN;
			assets->win.play();
		}
	}
	else if (mode == AITO) // "AIと対戦"用ループ
	{
#if defined(SPEED)
		int s = (CheckHitKey(KEY_INPUT_P)) ? 10 : 1;
		for (int cc = 0; cc < s; cc++)
		{
#endif
			// 1pがAIならここで思考する．
			if (f1->flag(WAIT_NEXT))
			{
				gs.push(GameState(*f1, *f2, *ai3con.operate(), *ai3con2.operate()));

				if (f1->flag(PLAYER_AI))
					ai3con.thinkWrapperEX(*f1, *f2);
				//	ai3con.thinkWrapper(*f1, *f2);
			}

			if (!f1->procedure(*f2, ai3con.operate()))
			{
				result = P2_WIN;
				assets->lose.play();
				assets->lose_voice.play();
#if defined(SPEED)
				break;
#endif
			};

			// ずるモードの時はこのタイミング
			if (f1->flag(ZURU_OK) && f1->flag(ZURU_MODE))
			{
				f1->tumoReload();
				putHelper(&gs);
			}

			if (f2->flag(WAIT_NEXT) && f2->flag(PLAYER_AI))
			{
				eval_called = 0;
				chain_hit = 0;
				chain_called = 0;
				node_searched = 0;
				HighPrecisionTimer tm;
				tm.begin();
				int score = 0;

				score = poly_ai.levelThink(*f2, *f1);
				//score = ai3con2.thinkWrapperEX(*f2, *f1);
				//ai3con2.thinkWrapper(*f2, *f1); // 旧AIメソッド

				tm.end();
				double elapsed = tm.elapsed();
				double nps = eval_called / elapsed;
				MyOutputDebugString("score = %d\nelapsed = %.10lfs\nnode = %d\nnps = %.2lf\nchain_hit = %d\nchain_called = %d\nnode_searched = %d\n",
					score, elapsed, eval_called, nps, chain_hit, chain_called, node_searched);

			}
			if (!f2->procedure(*f1, poly_ai.operate()))
			{
				result = P1_WIN;
				assets->win.play();
#if defined(SPEED)
				break;
#endif
			}
#if defined(SPEED)
		}
#endif
	}
	else
		assert(false);

	// spaceを押すと停止する（ずるをする）
	while(DxLib::CheckHitKey(KEY_INPUT_SPACE))
		WaitTimer(100);

	return result;
}

// リプレイモードで開始するかどうかを聞き、開始するなら初期化処理をする。
bool Game::selectedReplay()
{
	const std::string file_name = ONE_PLAY_LINE == 4 ? "history.txt" : "history2.txt";

	std::ifstream ifs(file_name);

	// ファイルに何件のリプレイがあるか表示
	int file_max, replay_num;

	std::string str;
	std::getline(ifs, str);
	std::stringstream ss(str);
	ss >> file_max;

	// コンソール画面の呼び出し
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);

	if (file_max)
	{
		do {
			std::cout << "リプレイファイルには" << file_max << "件のリプレイが保存されています。\n"
				<< "どのリプレイを再生しますか？\n" << "1～" << file_max << "までの数字を入力してください。\n"
				<< "0:やっぱりやめる\n";
			std::cin >> replay_num;
		} while (replay_num > file_max);
	}
	else
	{
		std::cout << "リプレイファイルがありません。リプレイモードを終了します。\n何かキーを入力してください。";
		getchar();
		replay_num = 0;
	}
	FreeConsole();

	// リプレイモードに移項しない場合はすべてこのifで引っかかる。
	if (replay_num == 0)
		return false;
	
	// リプレイモードでは、AIを使わない.ゲーム終了時(gameOver()内で)再びAIFlagをセットする処理をしている．
	f1->clearFlag(PLAYER_AI);
	f2->clearFlag(PLAYER_AI);

	// 指定されたリプレイまで読み飛ばし。1ゲームは4行で表される
	for (int i = 0; i < replay_num - 1; i++)
		for (int j = 0; j < ONE_PLAY_LINE; j++)
			std::getline(ifs, str);
		
#if ONE_PLAY_LINE == 5
	std::getline(ifs, str);
	if(!str.empty())
		assets->player_name = str;
#endif
	// 一行は1p、2行目は2pの操作履歴が入っている．3行目はお邪魔ぷよを降らすときに使う乱数の履歴．	
	ifs >> assets->operate_history_1p;
	ifs >> assets->operate_history_2p;
	ifs >> assets->ojama_history;

	std::getline(ifs, str);

	// バッファクリアとストリームの状態をクリアする。この行がないと意図通りに動作しない
	ss.str(""); 
	ss.clear(std::stringstream::goodbit);

	// 4行目はゲームで使われたツモ128種類．
	ss << str;

	for (int i = 0; i < 128; i++)
	{
		int c0, c1;
		ss >> c0 >> c1;
		assets->tumo_pool[i] = Tumo(DEADPOINT, static_cast<Color>(c0), static_cast<Color>(c1), ROTATE_UP);
	}

	return true;
}

bool Game::selectedZuru()
{
	int flag = MessageBox(
		NULL,
		TEXT("ずるモードでプレイしますか？"),
		TEXT("モード確認"),
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

	bool ret = (flag == IDYES);

	if (ret)
	{	
		SetWindowPosition(800, 250);

		// コンソール画面の呼び出し
		AllocConsole();
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONIN$", "r", stdin);
	}

	return ret;
}

// 128ツモの初期化
void Game::initTumo()
{

#if defined(LEARN)
	for (int i = 0; i < 128; i++)
	{
		const Color c1 = colorTypeToColor(static_cast<ColorType>(i % (NCOLOR_MAX - 1)));
		const Color c2 = colorTypeToColor(static_cast<ColorType>((i + 1) % (NCOLOR_MAX - 1)));
		
		// いつも同じぷよを出現させる
		assets->tumo_pool[i].init(DEADPOINT, c1, c2, ROTATE_UP);				
	}
	LightField::setUseColor(PURPLE);
#elif defined(LEARN2)
	for (int i = 0; i < 128; i++)
	{
		Color c1, c2;
		/*if (i >= 0 && i <= 3)
		{
			c1 = i & 1 ? RED : BLUE;
			c2 = i & 1 ? RED : BLUE;
		}
		else
		{*/
			c1 = colorTypeToColor(static_cast<ColorType>((i * SEEDA + SEEDB) % (NCOLOR_MAX - 1)));
			c2 = colorTypeToColor(static_cast<ColorType>((i * SEEDC + SEEDD) % (NCOLOR_MAX - 1)));
		//
		// いつも同じぷよを出現させる
		assets->tumo_pool[i].init(DEADPOINT, c1, c2, ROTATE_UP);				
	}
	LightField::setUseColor(PURPLE);
#elif defined(ZOROME)
	for (int i = 0; i < 128; i++)
	{
		const Color c1 = colorTypeToColor(static_cast<ColorType>(i % (NCOLOR_MAX - 1)));
		const Color c2 = colorTypeToColor(static_cast<ColorType>(i % (NCOLOR_MAX - 1)));

		// いつも同じぷよを出現させる
		assets->tumo_pool[i].init(DEADPOINT, c1, c2, ROTATE_UP);
	}
	LightField::setUseColor(PURPLE);
#else
	ColorType col[2];

	// 5色のうち選ばない色を決めておく。
	const ColorType except_color = random<ColorType>(NCOLOR_MAX);
	LightField::setUseColor(colorTypeToColor(except_color));

	for (int i = 0; i < TUMO_MAX; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			do {
				col[j] = random<ColorType>(NCOLOR_MAX);
			} while (col[j] == except_color);
		}
		assets->tumo_pool[i].init(DEADPOINT, col[0], col[1], ROTATE_UP);
	}
#endif

}

void Game::gameOver()
{
	if (isReplayMode()) // リプレイモードなら、AIを設定し直す
	{
		f2->setFlag(PLAYER_AI);
		f1->clearFlag(REPLAY_MODE);
		f2->clearFlag(REPLAY_MODE);
	}
	else // 通常のゲームモードなら、ゲームの履歴をファイルに出力
	{
#if defined AIVSAI
		saveGame();
		assets->operate_history_1p.clear();
		assets->operate_history_2p.clear();
		assets->ojama_history.clear();
#else
		saveGame();
#endif
	}
	reInit();

	if (isZuruMode())
	{
		FreeConsole();
		f1->clearFlag(ZURU_MODE);
		f2->clearFlag(ZURU_MODE);
	}
}

void Game::saveGame()
{
	const std::string history_file = "history.txt";

#if defined (AIVSAI)
	int flag = IDYES;
#else
	int flag = MessageBox(
		NULL,
		TEXT("リプレイを保存しますか？"),
		TEXT("リプレイ保存"),
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
#endif
	if (flag == IDYES)
	{
		// まず、先頭行のファイル数を書き換える
		std::fstream fst(history_file, std::ios::in | std::ios::out);
		if (fst)
		{
			int file_max;
			fst >> file_max;
			file_max++;
			fst.clear();
			fst.seekg(0);
			fst << file_max;
			fst.close();
		}

		// ファイル出力ストリームの初期化
		std::ofstream ofs(history_file, std::ios::app);

		// ファイルに1行ずつ書き込み
		ofs << assets->operate_history_1p;
		ofs << assets->operate_history_2p;
		ofs << assets->ojama_history;

		for (int i = 0; i < 128; i++)
			ofs << assets->tumo_pool[i].pColor() << " " << assets->tumo_pool[i].cColor() << " ";
		
		ofs << std::endl;
	}
}

void Game::ojamaRandInit()
{
	for (int i = 0; i < 128; i++)
		assets->ojama_rand_[i] = random<char>(5);
}

// ゲームを開始する前に呼び出す初期化処理
void Game::reInit()
{
	f1->init();
	f2->init();
	assets->operate_history_1p.clear();
	assets->operate_history_2p.clear();
	assets->ojama_history.clear();
	initTumo();
	ojamaRandInit();

	// 新しいゲームのために置換表をクリア
	TT.clear();
	CT.clear();
#ifdef AIVSAI
	f1->setFlag(PLAYER_AI);
#endif

	if (f1->flag(ZURU_MODE))
		FreeConsole();
}

void Game::fadeIn()
{
	static int br = 0;
	SetDrawBright(br, br, br);
	assets->field.draw();
	br += 40;

	if (br >= 255)
	{
		br = 0;
		SetDrawBright(255, 255, 255);
		fase = BORDER_APPEAR;
	}
}

void Game::borderAppear()
{
	static int i = 0;
	assets->field.draw();

	// 縮小表示
	DrawRotaGraph3(124, 253,
		assets->border.size().x / 2,
		assets->border.size().y / 2,
		1.0, 0.2 * i, 0, assets->border.handle(), TRUE, FALSE);

	if (i == 3)
	{
		// 再生周波数を変えて再生(レベル選択枠が出る音）
		assets->drop.changeFreq(70000);
		assets->drop.play(DX_PLAYTYPE_NORMAL);
	}

	i++;

	if (i >= 5)
	{
		i = 0;
		fase = LEVEL_SELECT;

		// 再生周波数を元に戻しておく
		assets->drop.changeFreq(-1);
	}
}

void Game::levelSelect()
{
	static const int x = assets->choose.getUpperLeft().x;
	static const int width = assets->choose.size().x;
	static const int height = assets->choose.size().y;
	static int y = assets->choose.getUpperLeft().y;
	static int level_1p = 3;
	static int timer = 0;

	// 最初はレベル3を選択
	assets->field.draw();
	assets->border.draw();

	if (CheckHitKey(KEY_INPUT_UP))// 上キー
	{
		if (level_1p > 1 && timer % 8 == 0)
		{
			level_1p--;
			y -= 38;
			assets->choose.setPosition(x, y);
			assets->cursor.play();
		}
		timer++;
	}
	else if (CheckHitKey(KEY_INPUT_DOWN))// 下キー
	{
		if (level_1p < 6 && timer % 8 == 0)
		{
			level_1p++;
			y += 38;
			assets->choose.setPosition(x, y);
			assets->cursor.play();
		}
		timer++;
	}
	else if (CheckHitKey(KEY_INPUT_1))// リプレイモード
	{
		assets->click.play(DX_PLAYTYPE_NORMAL);

		// リプレイ開始なら、次のモードへ
		if (selectedReplay())
		{
			f1->setFlag(REPLAY_MODE);
			f2->setFlag(REPLAY_MODE);
			fase = BORDER_DISAPPEAR;
		}
	}
	else if (CheckHitKey(KEY_INPUT_2))// ずるモード
	{
		assets->click.play(DX_PLAYTYPE_NORMAL);

		if (selectedZuru())
		{
			f1->setFlag(ZURU_MODE);
			f2->setFlag(ZURU_MODE);
			fase = BORDER_DISAPPEAR;
			ai3con2.setLevel(level_1p);
		}
	}
	else
	{
		timer = 0;
	}

	assets->choose.draw();

	// xキー:決定
	if (CheckHitKey(KEY_INPUT_X))
	{
		assets->click.play(DX_PLAYTYPE_NORMAL);
		fase = BORDER_DISAPPEAR;
		ai3con2.setLevel(level_1p);
		poly_ai.setLevel(level_1p);
	}

	// ぷよを表示
	f1->show();
	f2->show();
}

void Game::borderDisappear()
{
	// レベル選択の枠を少しずつ閉じる
	static int i = 5;
	assets->field.draw();
	DrawRotaGraph3(124, 253,
		assets->border.size().x / 2,
		assets->border.size().y / 2,
		1.0, 0.2 * i, 0, assets->border.handle(), TRUE, FALSE);

	if (--i <= 0)
	{
		fase = PLAYING;
		i = 5;
	}
}

void Game::playing(GameResult* result, const GameMode mode, int *p1_win, int* p2_win)
{
	// ゲーム中にQが押されたらレベル選択に戻る。
	if (CheckHitKey(KEY_INPUT_Q))
	{
		fase = BORDER_APPEAR;
		reInit();
		f1->clearFlag(REPLAY_MODE);
		f1->clearFlag(ZURU_MODE);
		f2->clearFlag(REPLAY_MODE);
		f2->clearFlag(ZURU_MODE);

		if(mode == AITO)
			f2->setFlag(PLAYER_AI);

		return;
	}

	*result = playLoop(mode);

	if (*result == P1_WIN)
	{
		(*p1_win)++;
		fase = RESULT;
	}
	else if (*result == P2_WIN)
	{
		(*p2_win)++;
		fase = RESULT;
	}
}

void Game::gameResult(const GameResult result, const int p1_win, const int p2_win)
{
	static int i = 0;
	assets->field.draw();

	if (result == P1_WIN)
	{
		assets->batankyuu.setPosition(438, 84);
		assets->batankyuu.draw();
		assets->yatta.setPosition(84, 84);
		assets->yatta.draw();
	}
	else if (result == P2_WIN)
	{
		assets->batankyuu.setPosition(84, 84);
		assets->batankyuu.draw();
		assets->yatta.setPosition(438, 84);
		assets->yatta.draw();
	}

	if (i++ >= 60)
	{
		
#if defined AIVSAI
		fase = PLAYING;
#else
		fase = BORDER_APPEAR;
#endif
		gameOver();
		i = 0;
	}
}

// 目的の場所まで最適な動作でおいてくれるヘルパー
void Game::putHelper(GameStack* gs)
{
	static const char* col[] = {"赤","緑","青","黄","紫"};
	const char* pivot_col = col[colorToColorType(f1->current().pColor())];
	const char* child_col = col[colorToColorType(f1->current().cColor())];
	int px, py, cx, cy, rotate;// y座標を入力する必要はなし
	bool tigiri;
	Move best;

	while(1)
	{
		tigiri = false;
		std::cout << "ツモ\n";

		// 10手先までのツモを表示する
		for (int i = 0; i < 10; i++)
		{
			int tumo_nb = (f1->nextTumoIndex() + i) % 128;
			std::cout << "(" << i << ")" << col[colorToColorType(assets->tumo_pool[tumo_nb].pColor())] 
										 << col[colorToColorType(assets->tumo_pool[tumo_nb].cColor())] << " ";
		}
		std::cout << "\n現在のツモは" << " 軸:" << pivot_col << " 子:" << child_col << "です。"
		<< "どこに置きますか？\n軸ぷよの位置、回転数を入力 待ったは0と入力\n" << "\nx:";
		std::cin >> px;

		if (std::cin.fail())
		{
			std::cout << "数字を入力\n"; 
			std::cin.clear();
			std::cin.ignore(1024, '\n');
			continue;
		}
		if (px == 0)
		{
			std::cout << gs->size() << std::endl;

			// 0手目で待ったをしたら空もありえる
			if (gs->empty())
			{
				std::cout << "前の状態が見つかりません\n";
			}
			else
			{
				// 待った
				*f1 = gs->top().f1;
				*f2 = gs->top().f2;
				*ai3con.operate() = gs->top().ope_1p;
				*ai3con2.operate() = gs->top().ope_2p;
			
				// 待ったのためだけに独自のループを作る。
				ClearDrawScreen();
				assets->field.draw();
				f1->show();
				f2->show();
				ScreenFlip();
				
				gs->pop();
			}
			continue;
		}

		std::cout << "\n回転数";
		std::cin >> rotate;
		if (std::cin.fail())
		{
			std::cout << "駄目です\n";
			std::cin.clear();
			std::cin.ignore(1024, '\n');
			continue;
		}
		
		if (!(px > 0 && px <= 6))
		{
			std::cout << "駄目です\n";
			continue;
		}

		if (rotate == 0)
		{
			py = f1->upper(px);
			cx = px;
			cy = py + 1;
		}
		else if (rotate == 2)
		{
			py = f1->upper(px) + 1;
			cx = px;
			cy = py - 1;
		}
		else if (rotate == 1)
		{
			py = f1->upper(px);
			cx = px + 1;
			if (!(cx > 0 && cx <= 6))
			{
				std::cout << "駄目です\n";
				continue;
			}
			cy = f1->upper(cx);
			tigiri = (py != cy);
		}
		else if (rotate == 3)
		{
			py = f1->upper(px);
			cx = px - 1;
			if (!(cx > 0 && cx <= 6))
			{
				std::cout << "駄目です\n";
				continue;
			}
			cy = f1->upper(cx);
			tigiri = (py != cy);
		}
		else
		{
			std::cout << "不正な回転数\n";
			continue;
		}

		best = Move(toSquare(px, py), toSquare(cx, cy), tigiri);

		if (!best.isLegal(*f1))
			std::cout << "そこには置けません。";
		else
			break;
	} 
	ai3con.operate()->generate(best, *f1);
}

// 勝ち星を表示
void Game::drawSummaryStar(const int p1_win, const int p2_win)
{
	if (p1_win)
	{
		// 1000勝するまでやる猛者はいないだろう。
		int win100 = p1_win / 100;
		int win20 = (p1_win % 100) / 20;
		int win1 = p1_win % 20;

		if (win100)
		{
			assets->win_star.setPosition(256, 237);
			assets->win_star.draw();

			if (win100 >= 2)
			{
				DxLib::DrawExtendGraph(264, 245, 281, 262, assets->win_num[win100].handle(), true);
			}

			if (win20)
			{
				assets->win_star.setPosition(288, 237);
				assets->win_star.draw();
				DxLib::DrawExtendGraph(288, 255, 305, 270, assets->win_num[win20 * 2].handle(), true);
				DxLib::DrawExtendGraph(303, 255, 320, 270, assets->win_num[0].handle(), true);
			}
		}
		else if (win20)
		{
			assets->win_star.setPosition(256, 237);
			assets->win_star.draw();
			DxLib::DrawExtendGraph(256, 255, 273, 270, assets->win_num[win20 * 2].handle(), true);
			DxLib::DrawExtendGraph(271, 255, 288, 270, assets->win_num[0].handle(), true);
		}

		for (int i = 0; i < win1; i++)
		{
			assets->star.setPosition(256 + ((i / 5) * 15), 269 + ((i % 5) * 15));
			assets->star.draw();
		}
	}

	if (p2_win)
	{
		int win100 = p2_win / 100;
		int win20 = (p2_win % 100) / 20;
		int win1 = p2_win % 20;

		if (win100)
		{
			assets->win_star.setPosition(354, 237);
			assets->win_star.draw();

			if (win100 >= 2)
			{
				DxLib::DrawExtendGraph(362, 245, 379, 262, assets->win_num[win100].handle(), true);
			}
			if (win20)
			{
				assets->win_star.setPosition(322, 237);
				assets->win_star.draw();
				DxLib::DrawExtendGraph(322, 255, 339, 270, assets->win_num[win20 * 2].handle(), true);
				DxLib::DrawExtendGraph(337, 255, 354, 270, assets->win_num[0].handle(), true);
			}
		}
		else if (win20)
		{
			assets->win_star.setPosition(354, 237);
			assets->win_star.draw();
			DxLib::DrawExtendGraph(354, 255, 371, 270, assets->win_num[win20 * 2].handle(), true);
			DxLib::DrawExtendGraph(369, 255, 386, 270, assets->win_num[0].handle(), true);
		}

		for (int i = 0; i < win1; i++)
		{
			assets->star.setPosition(373 - ((i / 5) * 15), 269 + ((i % 5) * 15));
			assets->star.draw();
		}
	}
}
