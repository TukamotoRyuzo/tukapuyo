#include "gamemode.h"
#include <cmath>
#include <cfloat>
#include "GameAssets.h"

namespace MainMenu
{
	enum Fase { NO_MOTION, SELECTED, FADEOUT };
	
	Vector vec[GAMEMODE_MAX];
	
	GameAssets* assets;

	void hop(GameMode);
	void flash(GameMode);
}

void MainMenu::init(GameAssets* s)
{
	assets = s;

	// menu画面に必要な画像，音情報のみを初期化
	assets->menuInit();

	// ボタンの座標は保持しておく。
	vec[0] = assets->button[0].getUpperLeft();
	vec[1] = assets->button[1].getUpperLeft();
	vec[2] = assets->button[2].getUpperLeft();
}

int MainMenu::loop()
{
	Fps fps;

	GameMode id = TOKOTON;
	Fase fase = NO_MOTION;

	// BGMを鳴らす
	assets->bgm_menu.play(DX_PLAYTYPE_LOOP);

	while(!ScreenFlip() && !ProcessMessage() && !ClearDrawScreen())
	{
		// menu画面の表示
		assets->menu.draw();

		if (CheckHitKey(KEY_INPUT_ESCAPE))
			return 1;

		switch(fase)
		{
		case NO_MOTION: // 何のモーションもないとき
			{
				if (CheckHitKey(KEY_INPUT_X)) // xが押されたら決定
				{
					// 次のモードへ
					fase = SELECTED;
					assets->click.play(DX_PLAYTYPE_BACK);
					break;
				}

				// キーが効きすぎる問題への対策
				static int timer = 0;

				if (CheckHitKey(KEY_INPUT_DOWN)) // ↑キー，↓キーで選択
				{
					if (id < AITO && timer % 8 == 0) 
					{
						++id;
						assets->cursor.play(DX_PLAYTYPE_BACK);
					}
					timer++;
				}
				else if (CheckHitKey(KEY_INPUT_UP))
				{
					if (id > TOKOTON && timer % 8 == 0)
					{
						--id;
						assets->cursor.play(DX_PLAYTYPE_BACK);
					}
					timer++;
				}
				else timer = 0;
				
				// 選択されている画像がぴょんぴょんはねる
				hop(id);
				break;
			}
		case SELECTED: // ボタンが押されたとき
			flash(id);
			fase = FADEOUT;
			break;

		case FADEOUT: // フェードアウト処理

			for (int i = 255; i >= 0 && !ScreenFlip() && !ProcessMessage() &&!ClearDrawScreen(); i -= 5)
			{
				SetDrawBright(i, i, i);
				assets->menu.draw();

				for (int i = 0; i < 3; i++)
					assets->button[i].draw();

				fps.update();
				fps.wait();
			}
			WaitTimer(300);

			// bgmのストップ
			assets->bgm_menu.stop();

			// メモリ解放
			assets->menuFinal();
			
			// ゲーム中に使う音声、画像を読み込む
			Game::init(assets);

			// ゲームループへ
			if (Game::loop(id) == -1)
				return -1;

			// 画像，音を読み込みなおす
			assets->menuInit();

			assets->bgm_menu.play(DX_PLAYTYPE_LOOP);

			fase = NO_MOTION;
			break;

		default:
			return -1;
		}

		fps.update(); // 更新
		fps.draw();
		fps.wait(); // 待機
	}
	return 0;
}

// id   0: とことん
// 		1: ふたりで
// 		2: ＡＩと
void MainMenu::hop(GameMode id)
{
	const double x = vec[id].x;
	const double y = vec[id].y;
	const int width  = static_cast<int>(assets->button[id].size().x);
	const int height = static_cast<int>(assets->button[id].size().y);

	static double dy = 0;

	for (GameMode i = TOKOTON; i <= AITO; ++i)
		if (i != id)
			assets->button[i].draw();
	
	dy += 0.15;

	// 念のためにオーバーフロー対策
	// オーバーフローする心配はおそらくないが一応．
	if (dy > 100000.0f) dy = 0;

	// sinのy座標がはねるように遷移することを利用する
	assets->button_t[id].setPosition((int)x, int(y - 10 * abs(sin(dy))));	
	assets->button_t[id].draw();
}

void MainMenu::flash(GameMode id)
{
	bool t = false;
	
	// 表示位置を元に戻しておく
	for (int i = 0; i < 3; i++)
	{
		assets->button[i].setPosition(vec[i].x, vec[i].y);
		assets->button_t[i].setPosition(vec[i].x, vec[i].y);
	}

	// 点滅
	for (int i = 0; i < 10 && !ProcessMessage() && !ClearDrawScreen(); i++)
	{
		assets->menu.draw();

		for (GameMode j = TOKOTON; j <= AITO; ++j)
			if (j != id)
			assets->button[j].draw();

		if (t)
		{
			assets->button_t[id].draw();
			t = false;
		}
		else
		{
			assets->button[id].draw();
			t = true;
		}

		ScreenFlip();
		WaitTimer(40);
	}
}