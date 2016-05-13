#include <ctime>
#include "GameAssets.h"
#include "gamemode.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetAlwaysRunFlag(TRUE);
	// SetWaitVSyncFlag(FALSE) ;

	// �c�w���C�u��������������
	if (DxLib_Init() == -1) 
		return -1;

	srand((unsigned int)time(NULL));
	//srand((unsigned int)24);

	// �J�����g�f�B���N�g���̕ύX
#if _MSC_VER == 1600 || _MSC_VER == 1800
	SetCurrentDirectory("C:\\Users\\ryuzo\\Dropbox\\Tukapuyo_src");
#elif _MSC_VER == 1900
	SetCurrentDirectory("E:\\Dropbox\\Tukapuyo_src");
#else
	SetCurrentDirectory("C:\\Users\\eits1419\\Dropbox\\Tukapuyo_src");
#endif

	 // �uAlt+Enter�v��A�ő剻�{�^���ɂ��t���X�N���[������L���ɂ���
   // SetUseASyncChangeWindowModeFunction(TRUE, NULL, NULL);
    
	// �t���X�N���[���ł͂Ȃ����[�h
	ChangeWindowMode(TRUE);
	//ChangeWindowMode(FALSE);

	// �`���𗠉�ʂ�
	SetDrawScreen(DX_SCREEN_BACK);

	GameAssets* s = new GameAssets;

	MainMenu::init(s);
	
	if (MainMenu::loop() == -1)
		return -1;
	
	// �c�w���C�u�����g�p�̏I������
	DxLib_End() ;				

	return 0;
}
