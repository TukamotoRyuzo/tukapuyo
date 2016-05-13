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

	// menu��ʂɕK�v�ȉ摜�C�����݂̂�������
	assets->menuInit();

	// �{�^���̍��W�͕ێ����Ă����B
	vec[0] = assets->button[0].getUpperLeft();
	vec[1] = assets->button[1].getUpperLeft();
	vec[2] = assets->button[2].getUpperLeft();
}

int MainMenu::loop()
{
	Fps fps;

	GameMode id = TOKOTON;
	Fase fase = NO_MOTION;

	// BGM��炷
	assets->bgm_menu.play(DX_PLAYTYPE_LOOP);

	while(!ScreenFlip() && !ProcessMessage() && !ClearDrawScreen())
	{
		// menu��ʂ̕\��
		assets->menu.draw();

		if (CheckHitKey(KEY_INPUT_ESCAPE))
			return 1;

		switch(fase)
		{
		case NO_MOTION: // ���̃��[�V�������Ȃ��Ƃ�
			{
				if (CheckHitKey(KEY_INPUT_X)) // x�������ꂽ�猈��
				{
					// ���̃��[�h��
					fase = SELECTED;
					assets->click.play(DX_PLAYTYPE_BACK);
					break;
				}

				// �L�[��������������ւ̑΍�
				static int timer = 0;

				if (CheckHitKey(KEY_INPUT_DOWN)) // ���L�[�C���L�[�őI��
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
				
				// �I������Ă���摜���҂��҂��͂˂�
				hop(id);
				break;
			}
		case SELECTED: // �{�^���������ꂽ�Ƃ�
			flash(id);
			fase = FADEOUT;
			break;

		case FADEOUT: // �t�F�[�h�A�E�g����

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

			// bgm�̃X�g�b�v
			assets->bgm_menu.stop();

			// ���������
			assets->menuFinal();
			
			// �Q�[�����Ɏg�������A�摜��ǂݍ���
			Game::init(assets);

			// �Q�[�����[�v��
			if (Game::loop(id) == -1)
				return -1;

			// �摜�C����ǂݍ��݂Ȃ���
			assets->menuInit();

			assets->bgm_menu.play(DX_PLAYTYPE_LOOP);

			fase = NO_MOTION;
			break;

		default:
			return -1;
		}

		fps.update(); // �X�V
		fps.draw();
		fps.wait(); // �ҋ@
	}
	return 0;
}

// id   0: �Ƃ��Ƃ�
// 		1: �ӂ����
// 		2: �`�h��
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

	// �O�̂��߂ɃI�[�o�[�t���[�΍�
	// �I�[�o�[�t���[����S�z�͂����炭�Ȃ����ꉞ�D
	if (dy > 100000.0f) dy = 0;

	// sin��y���W���͂˂�悤�ɑJ�ڂ��邱�Ƃ𗘗p����
	assets->button_t[id].setPosition((int)x, int(y - 10 * abs(sin(dy))));	
	assets->button_t[id].draw();
}

void MainMenu::flash(GameMode id)
{
	bool t = false;
	
	// �\���ʒu�����ɖ߂��Ă���
	for (int i = 0; i < 3; i++)
	{
		assets->button[i].setPosition(vec[i].x, vec[i].y);
		assets->button_t[i].setPosition(vec[i].x, vec[i].y);
	}

	// �_��
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