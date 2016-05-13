#pragma once

#include <string>
// リープモーションを使う場合
// #define USE_LEAPMOTION
#ifdef USE_LEAPMOTION
#include"Leap.h"
#endif

std::string nameboad();

struct GameAssets;
enum GameMode;
namespace MainMenu
{
	void init(GameAssets* s);
	int loop();
}

enum GameResult { NO_RESULT, P1_WIN, P2_WIN, GAME_ERROR };

namespace Game
{
	void init(GameAssets* s);
	int loop(const GameMode mode);
}