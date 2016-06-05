#pragma once

#include <string>

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