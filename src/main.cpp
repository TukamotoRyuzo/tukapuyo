#include <ctime>
#include "GameAssets.h"
#include "gamemode.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetAlwaysRunFlag(TRUE);
	// SetWaitVSyncFlag(FALSE) ;

	// ＤＸライブラリ初期化処理
	if (DxLib_Init() == -1) 
		return -1;

	srand((unsigned int)time(NULL));
	//srand((unsigned int)24);

	// カレントディレクトリの変更
#if _MSC_VER == 1600 || _MSC_VER == 1800
	SetCurrentDirectory("C:\\Users\\ryuzo\\Dropbox\\Tukapuyo_src");
#elif _MSC_VER == 1900
	SetCurrentDirectory("E:\\Dropbox\\Tukapuyo_src");
#else
	SetCurrentDirectory("C:\\Users\\eits1419\\Dropbox\\Tukapuyo_src");
#endif

	 // 「Alt+Enter」や、最大化ボタンによるフルスクリーン化を有効にする
   // SetUseASyncChangeWindowModeFunction(TRUE, NULL, NULL);
    
	// フルスクリーンではないモード
	ChangeWindowMode(TRUE);
	//ChangeWindowMode(FALSE);

	// 描画先を裏画面に
	SetDrawScreen(DX_SCREEN_BACK);

	GameAssets* s = new GameAssets;

	MainMenu::init(s);
	
	if (MainMenu::loop() == -1)
		return -1;
	
	// ＤＸライブラリ使用の終了処理
	DxLib_End() ;				

	return 0;
}
