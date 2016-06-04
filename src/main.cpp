#include <ctime>
#include "GameAssets.h"
#include "gamemode.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	ChangeWindowMode(TRUE);
	SetAlwaysRunFlag(TRUE);

	// �c�w���C�u��������������
	if (DxLib_Init() == -1) 
		return -1;

	srand((unsigned int)time(NULL));
	//srand((unsigned int)24);

	// �`���𗠉�ʂ�
	SetDrawScreen(DX_SCREEN_BACK);

	GameAssets* s = new GameAssets;

	MainMenu::init(s);
	
	if (MainMenu::loop() == -1)
		return -1;
	
	DxLib_End();				

	return 0;
}
