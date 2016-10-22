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
#define AILEVEL_1p 5
#define ONE_PLAY_LINE 4

#define GAME_NUM 2

enum
{
	SEEDA = 1445, SEEDB = 3, SEEDC = 7, SEEDD = 349,
};

// �Q�[���̏�Ԃ������ɃR�s�[���Ă����΁u�҂����v���\�ɂȂ�B
struct GameState
{
	Field f1, f2;
	Operate ope_1p, ope_2p;
	GameState(const Field f_1, const Field f_2, const Operate op1, const Operate op2) : f1(f_1), f2(f_2), ope_1p(op1), ope_2p(op2) {};
};

typedef std::stack<GameState> GameStack;

// �Ƃ��Ƃ�Ɠ�l�ŁAAI���󂯎��� 
namespace Game
{
	enum Fase { FADEIN, BORDER_APPEAR, LEVEL_SELECT, PLAYING, BORDER_DISAPPEAR, RESULT };
	
	Fase fase;

	// �摜�̍\����
	GameAssets* assets;

	// AI
	AI3Connect2 ai3con(AILEVEL_1p);
	AI3Connect2 ai3con2(3);
	PolytecAI poly_ai;

	// 1p, 2p�̃t�B�[���h
	Field *f1, *f2;

	// �eFase�ɑΉ����ČĂяo�����
	void fadeIn();
	void borderAppear();
	void levelSelect();
	void playing(GameResult* result, const GameMode mode, int *p1_win, int* p2_win);
	void borderDisappear();
	void gameResult(const GameResult result, const int p1_win, const int p2_win);

	// Fase == PLAYING�̎��ɌĂяo����郋�[�v�D
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

// �Q�[���ɕK�v�ȉ摜�C�c���̏������C�t�B�[���h�̏��������s���D
void Game::init(GameAssets* s)
{
	assets = s;
	assets->gameInit();

	f1 = new Field(PLAYER1, assets);
	f2 = new Field(PLAYER2, assets);

	// 200MB�̋ǖʕ\���m��
	TT.resize(200);
	CT.setSize(200);
	
	// �c���̏������ƂՂ�Ղ�̓������̏�����
	reInit();
}

// ���C�����[�v�D
int Game::loop(const GameMode mode)
{
	Fps fps;
	GameResult result = NO_RESULT;
	int p1_win = 0;
	int p2_win = 0;
	fase = FADEIN;

	// 2P����AI�ɂ��邩�ۂ��B
	if (mode == AITO)
		f2->setFlag(PLAYER_AI);
	else
		f2->clearFlag(PLAYER_AI);

	// bgm�Đ�(��������̂��񂪂�)
	assets->bgm_battle.play(DX_PLAYTYPE_LOOP);

	// ���C�����[�v
	while(!ScreenFlip() && !ProcessMessage() && !ClearDrawScreen())
	{

#if !defined POLYTEC_FESTA
		// esc�L�[�������ꂽ�烁�j���[��ʂɖ߂�
		if (CheckHitKey(KEY_INPUT_E))
		{
			reInit();
			assets->bgm_battle.stop();
			assets->gameFinal();
			return 0;
		}
#endif
#if defined POLYTEC_FESTA

		// �ǂ��炩��2�{��悵���炨���
		if (p1_win == GAME_NUM || p2_win == GAME_NUM)
		{
			bool win = p1_win == GAME_NUM;

			if (win)
				assets->you_win.draw();
			else
				assets->you_lose.draw();

			int level = ai3con2.getLevel();
			assets->chain_num[level].setPosition(win ? 575 : 570, win ? 105 : 135);
			assets->chain_num[level].draw();

			// R�������ꂽ��Q�[�����ĊJ����D�������܂ł͑���s�\�D
			// ����͓����C�^�c�������ʉ�ʂ��m�F������ŁC���̂��q���񂪒��킷��Ƃ��ɉ^�c�ɉ����Ă��炤�D
			// ���q���񑤂̓R���g���[�������������C���q���񑤂���͉����Ȃ��悤�ɂ���D
			if (CheckHitKey(KEY_INPUT_R))
			{
				p1_win = p2_win = 0;
				assets->click.play();
				reInit();
			}

			// ���[�v�I���Ȃ̂�fps��60�ɕۂ��߂ɂ����ŌĂяo���D
			fps.update();
			fps.wait();
			continue;
		}
#endif
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

		// ��������\������B
		drawSummaryStar(p1_win, p2_win);

		fps.update();
		fps.wait();
	}

	return 0;
}

// �v���C���̃��[�v�D
GameResult Game::playLoop(const GameMode mode)
{
	GameResult result = NO_RESULT;
	static GameStack gs;

	assets->field.draw();

	if (mode == TOKOTON) // "�Ƃ��Ƃ�"�p���[�v
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
			MyOutputDebugString("�����Ă���A���̓_�� = %d, node = %d, ���Ԃ� = %d, elapsed = %.10lfs\n", ss, color_searched, allready_searched, tm.elapsed());
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
	else if (mode == HUTARIDE) // "�ӂ����"�p���[�v
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
	else if (mode == AITO) // "AI�Ƒΐ�"�p���[�v
	{
#if defined(SPEED)
//#ifdef POLYTEC_FESTA
//		int s = 1;
//#else
		int s = (CheckHitKey(KEY_INPUT_P)) ? 10 : 1;
//#endif
		for (int cc = 0; cc < s; cc++)
		{
#endif
			// 1p��AI�Ȃ炱���Ŏv�l����D
			if (f1->flag(WAIT_NEXT))
			{
				gs.push(GameState(*f1, *f2, *ai3con.operate(), *ai3con2.operate()));

				if (f1->flag(PLAYER_AI))
				//	ai3con.thinkWrapperEX(*f1, *f2);
					ai3con.thinkWrapper(*f1, *f2);
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

			// ���郂�[�h�̎��͂��̃^�C�~���O
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
				//ai3con2.thinkWrapper(*f2, *f1); // ��AI���\�b�h

				tm.end();
				double elapsed = tm.elapsed();
				double nps = eval_called / elapsed;
				//MyOutputDebugString("score = %d\nelapsed = %.10lfs\nnode = %d\nnps = %.2lf\nchain_hit = %d\nchain_called = %d\nnode_searched = %d\n",
					//score, elapsed, eval_called, nps, chain_hit, chain_called, node_searched);

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

	// space�������ƒ�~����i���������j
	while(DxLib::CheckHitKey(KEY_INPUT_SPACE))
		WaitTimer(100);

	return result;
}

// ���v���C���[�h�ŊJ�n���邩�ǂ����𕷂��A�J�n����Ȃ珉��������������B
bool Game::selectedReplay()
{
	const std::string file_name = ONE_PLAY_LINE == 4 ? "history.txt" : "history2.txt";

	std::ifstream ifs(file_name);

	// �t�@�C���ɉ����̃��v���C�����邩�\��
	int file_max, replay_num;

	std::string str;
	std::getline(ifs, str);
	std::stringstream ss(str);
	ss >> file_max;

	// �R���\�[����ʂ̌Ăяo��
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);

	if (file_max)
	{
		do {
			std::cout << "���v���C�t�@�C���ɂ�" << file_max << "���̃��v���C���ۑ�����Ă��܂��B\n"
				<< "�ǂ̃��v���C���Đ����܂����H\n" << "1�`" << file_max << "�܂ł̐�������͂��Ă��������B\n"
				<< "0:����ς��߂�\n";
			std::cin >> replay_num;
		} while (replay_num > file_max);
	}
	else
	{
		std::cout << "���v���C�t�@�C��������܂���B���v���C���[�h���I�����܂��B\n�����L�[����͂��Ă��������B";
		getchar();
		replay_num = 0;
	}
	FreeConsole();

	// ���v���C���[�h�Ɉڍ����Ȃ��ꍇ�͂��ׂĂ���if�ň���������B
	if (replay_num == 0)
		return false;
	
	// ���v���C���[�h�ł́AAI���g��Ȃ�.�Q�[���I����(gameOver()����)�Ă�AIFlag���Z�b�g���鏈�������Ă���D
	f1->clearFlag(PLAYER_AI);
	f2->clearFlag(PLAYER_AI);

	// �w�肳�ꂽ���v���C�܂œǂݔ�΂��B1�Q�[����4�s�ŕ\�����
	for (int i = 0; i < replay_num - 1; i++)
		for (int j = 0; j < ONE_PLAY_LINE; j++)
			std::getline(ifs, str);
		
#if ONE_PLAY_LINE == 5
	std::getline(ifs, str);
	if(!str.empty())
		assets->player_name = str;
#endif
	// ��s��1p�A2�s�ڂ�2p�̑��엚���������Ă���D3�s�ڂ͂��ז��Ղ���~�炷�Ƃ��Ɏg�������̗����D	
	ifs >> assets->operate_history_1p;
	ifs >> assets->operate_history_2p;
	ifs >> assets->ojama_history;

	std::getline(ifs, str);

	// �o�b�t�@�N���A�ƃX�g���[���̏�Ԃ��N���A����B���̍s���Ȃ��ƈӐ}�ʂ�ɓ��삵�Ȃ�
	ss.str(""); 
	ss.clear(std::stringstream::goodbit);

	// 4�s�ڂ̓Q�[���Ŏg��ꂽ�c��128��ށD
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
		TEXT("���郂�[�h�Ńv���C���܂����H"),
		TEXT("���[�h�m�F"),
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

	bool ret = (flag == IDYES);

	if (ret)
	{	
		SetWindowPosition(800, 250);

		// �R���\�[����ʂ̌Ăяo��
		AllocConsole();
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONIN$", "r", stdin);
	}

	return ret;
}

// 128�c���̏�����
void Game::initTumo()
{

#if defined(LEARN)
	for (int i = 0; i < 128; i++)
	{
		const Color c1 = colorTypeToColor(static_cast<ColorType>(i % (NCOLOR_MAX - 1)));
		const Color c2 = colorTypeToColor(static_cast<ColorType>((i + 1) % (NCOLOR_MAX - 1)));
		
		// ���������Ղ���o��������
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
		// ���������Ղ���o��������
		assets->tumo_pool[i].init(DEADPOINT, c1, c2, ROTATE_UP);				
	}
	LightField::setUseColor(PURPLE);
#elif defined(ZOROME)
	for (int i = 0; i < 128; i++)
	{
		const Color c1 = colorTypeToColor(static_cast<ColorType>(i % (NCOLOR_MAX - 1)));
		const Color c2 = colorTypeToColor(static_cast<ColorType>(i % (NCOLOR_MAX - 1)));

		// ���������Ղ���o��������
		assets->tumo_pool[i].init(DEADPOINT, c1, c2, ROTATE_UP);
	}
	LightField::setUseColor(PURPLE);
#else
	ColorType col[2];

	// 5�F�̂����I�΂Ȃ��F�����߂Ă����B
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
	if (isReplayMode()) // ���v���C���[�h�Ȃ�AAI��ݒ肵����
	{
		f2->setFlag(PLAYER_AI);
		f1->clearFlag(REPLAY_MODE);
		f2->clearFlag(REPLAY_MODE);
	}
	else // �ʏ�̃Q�[�����[�h�Ȃ�A�Q�[���̗������t�@�C���ɏo��
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
#elif !defined POLYTEC_FESTA
	int flag = MessageBox(
		NULL,
		TEXT("���v���C��ۑ����܂����H"),
		TEXT("���v���C�ۑ�"),
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
#else
	int flag = IDYES;
#endif

	if (flag == IDYES)
	{
		// �܂��A�擪�s�̃t�@�C����������������
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

		// �t�@�C���o�̓X�g���[���̏�����
		std::ofstream ofs(history_file, std::ios::app);

		// �t�@�C����1�s����������
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

// �Q�[�����J�n����O�ɌĂяo������������
void Game::reInit()
{
	f1->init();
	f2->init();
	assets->operate_history_1p.clear();
	assets->operate_history_2p.clear();
	assets->ojama_history.clear();
	initTumo();
	ojamaRandInit();

	// �V�����Q�[���̂��߂ɒu���\���N���A
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

	// �k���\��
	DrawRotaGraph3(124, 253,
		assets->border.size().x / 2,
		assets->border.size().y / 2,
		1.0, 0.2 * i, 0, assets->border.handle(), TRUE, FALSE);

	if (i == 3)
	{
		// �Đ����g����ς��čĐ�(���x���I��g���o�鉹�j
		assets->drop.changeFreq(70000);
		assets->drop.play(DX_PLAYTYPE_NORMAL);
	}

	i++;

	if (i >= 5)
	{
		i = 0;
		fase = LEVEL_SELECT;

		// �Đ����g�������ɖ߂��Ă���
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

	// �ŏ��̓��x��3��I��
	assets->field.draw();
	assets->border.draw();

	if (CheckHitKey(KEY_INPUT_UP))// ��L�[
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
	else if (CheckHitKey(KEY_INPUT_DOWN))// ���L�[
	{
		if (level_1p < 5 && timer % 8 == 0)
		{
			level_1p++;
			y += 38;
			assets->choose.setPosition(x, y);
			assets->cursor.play();
		}
		timer++;
	}
	else if (CheckHitKey(KEY_INPUT_1))// ���v���C���[�h
	{
		assets->click.play(DX_PLAYTYPE_NORMAL);

		// ���v���C�J�n�Ȃ�A���̃��[�h��
		if (selectedReplay())
		{
			f1->setFlag(REPLAY_MODE);
			f2->setFlag(REPLAY_MODE);
			fase = BORDER_DISAPPEAR;
		}
	}
	else if (CheckHitKey(KEY_INPUT_2))// ���郂�[�h
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

	// x�L�[:����
	if (CheckHitKey(KEY_INPUT_X))
	{
		assets->click.play(DX_PLAYTYPE_NORMAL);
		fase = BORDER_DISAPPEAR;
		ai3con2.setLevel(level_1p);
		poly_ai.setLevel(level_1p);
	}

	// �Ղ��\��
	f1->show();
	f2->show();
}

void Game::borderDisappear()
{
	// ���x���I���̘g������������
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

#if !defined POLYTEC_FESTA
	// �Q�[������Q�������ꂽ�烌�x���I���ɖ߂�B
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

#endif

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

		if (p1_win < GAME_NUM && p2_win < GAME_NUM && (p1_win || p2_win))
		{
			MessageBox(
				NULL,
				TEXT("�ĊJ���܂��B���{�^���������Ă��������B"),
				TEXT("�������񂫂イ����"),
				MB_OK | MB_ICONQUESTION);

			fase = PLAYING;
		}
	}
}

// �ړI�̏ꏊ�܂ōœK�ȓ���ł����Ă����w���p�[
void Game::putHelper(GameStack* gs)
{
	static const char* col[] = {"��","��","��","��","��"};
	const char* pivot_col = col[colorToColorType(f1->current().pColor())];
	const char* child_col = col[colorToColorType(f1->current().cColor())];
	int px, py, cx, cy, rotate;// y���W����͂���K�v�͂Ȃ�
	bool tigiri;
	Move best;

	while(1)
	{
		tigiri = false;
		std::cout << "�c��\n";

		// 10���܂ł̃c����\������
		for (int i = 0; i < 10; i++)
		{
			int tumo_nb = (f1->nextTumoIndex() + i) % 128;
			std::cout << "(" << i << ")" << col[colorToColorType(assets->tumo_pool[tumo_nb].pColor())] 
										 << col[colorToColorType(assets->tumo_pool[tumo_nb].cColor())] << " ";
		}
		std::cout << "\n���݂̃c����" << " ��:" << pivot_col << " �q:" << child_col << "�ł��B"
		<< "�ǂ��ɒu���܂����H\n���Ղ�̈ʒu�A��]������� �҂�����0�Ɠ���\n" << "\nx:";
		std::cin >> px;

		if (std::cin.fail())
		{
			std::cout << "���������\n"; 
			std::cin.clear();
			std::cin.ignore(1024, '\n');
			continue;
		}
		if (px == 0)
		{
			std::cout << gs->size() << std::endl;

			// 0��ڂő҂����������������肦��
			if (gs->empty())
			{
				std::cout << "�O�̏�Ԃ�������܂���\n";
			}
			else
			{
				// �҂���
				*f1 = gs->top().f1;
				*f2 = gs->top().f2;
				*ai3con.operate() = gs->top().ope_1p;
				*ai3con2.operate() = gs->top().ope_2p;
			
				// �҂����̂��߂����ɓƎ��̃��[�v�����B
				ClearDrawScreen();
				assets->field.draw();
				f1->show();
				f2->show();
				ScreenFlip();
				
				gs->pop();
			}
			continue;
		}

		std::cout << "\n��]��";
		std::cin >> rotate;
		if (std::cin.fail())
		{
			std::cout << "�ʖڂł�\n";
			std::cin.clear();
			std::cin.ignore(1024, '\n');
			continue;
		}
		
		if (!(px > 0 && px <= 6))
		{
			std::cout << "�ʖڂł�\n";
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
				std::cout << "�ʖڂł�\n";
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
				std::cout << "�ʖڂł�\n";
				continue;
			}
			cy = f1->upper(cx);
			tigiri = (py != cy);
		}
		else
		{
			std::cout << "�s���ȉ�]��\n";
			continue;
		}

		best = Move(toSquare(px, py), toSquare(cx, cy), tigiri);

		if (!best.isLegal(*f1))
			std::cout << "�����ɂ͒u���܂���B";
		else
			break;
	} 
	ai3con.operate()->generate(best, *f1);
}

// ��������\��
void Game::drawSummaryStar(const int p1_win, const int p2_win)
{
	if (p1_win)
	{
		// 1000������܂ł��Ҏ҂͂��Ȃ����낤�B
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
