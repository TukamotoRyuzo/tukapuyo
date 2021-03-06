#include "GameAssets.h"
#include "gamemode.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// no output log.txt
	SetOutApplicationLogValidFlag(FALSE);
	SetAlwaysRunFlag(TRUE);
	ChangeWindowMode(TRUE);
	SetMainWindowText("tukapuyo");
	//SetWindowSizeExtendRate(2.0);

	// ＤＸライブラリ初期化処理
	if (DxLib_Init() == -1) 
		return -1;

	// 描画先を裏画面に
	SetDrawScreen(DX_SCREEN_BACK);

	GameAssets* s = new GameAssets;

	MainMenu::init(s);
	
	if (MainMenu::loop() == -1)
		return -1;
	
	DxLib_End();				

	return 0;
}
