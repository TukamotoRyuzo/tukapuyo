#include "dxlib.h"
#include "iostream"
#include "gamemode.h"
std::string nameboad(){

	
	const int RL = 32;//�J�[�\���ړ�right,left

	const int UD = 32;//�J�[�\���ړ�up,down

	int setx = 288;//�J�[�\��x�l(�����ʒu:��-��288)
	const int startx = setx;

	int sety = 244;//�J�[�\��y�l(�����ʒu:��-��244)
	const int starty = sety;
	//6�~11
	const int maxR1 = setx+RL;//�J�[�\���ő�ʒu
	const int maxL1 = setx-RL*9;//�J�[�\���ő�ʒu
	const int maxU1 = sety;//�J�[�\���ő�ʒu
	const int maxD1 = sety+UD*5;//�J�[�\���ő�ʒu


	int maxR;//�J�[�\���ő�ʒu
	int maxL;//�J�[�\���ő�ʒu
	int maxU;//�J�[�\���ő�ʒu
	int maxD;//�J�[�\���ő�ʒu






	std::string name2="";//return�l�ł���string�^�����z��(�I���������O)
	char key[120]="";//�I������������K�X�\�����邽�߂�char�^����
	
	int count = 0;//�܏\���\����Ђ炪�Ȃ̑I��

	int n = 0;//�����\���̔z����̕�����

	int w = 0;//�{�^���̒���������

	int edfl = 0;//�I���t���O

	int lastcount[20];//������ł����񂾎���count�̗���

	int lastlang[20];//�ł����񂾕����̌���̗���

	int lastcount2[20];//�����t���O

	int langsel = 0;//0:�Ђ炪�� 1:�J�^�J�i 2:english

	int max = 10;//�ő啶����

	int loopcount=0;

	//�摜
	const int GH1 = LoadGraph( "picture\\backboad.png" );//�w�i�̔�
	const int GH2 = LoadGraph( "picture\\yerrow.png" );//�J�[�\���\��
	const int GH3 = LoadGraph( "picture\\keyboad-����ver3.png" );//�Ђ炪�ȃL�[�{�[�h
	const int GH4 = LoadGraph( "picture\\enter_yerrow.png" );//�J�[�\���\�� 
	const int GH5 = LoadGraph( "picture\\return_yerrow.png" );//�J�[�\���\�� 
	const int GH6 = LoadGraph( "picture\\ggg.png" ) ;//���O�\�� 
	const int GH7 = LoadGraph( "picture\\�w�i_vertest.png" ) ;//�w�i�\��
	const int GH8 = LoadGraph( "picture\\keyboad-kana.png" );//�J�^�J�i�L�[�{�[�h
	const int GH9 = LoadGraph( "picture\\keyboad-english.png" );//english�L�[�{�[�h
	const int GH10= LoadGraph( "picture\\wordline.png" ) ;//�������C���\��


	char word1[160] = { "�����������[�����������������������傽���ĂƂ�Ȃɂʂ˂̂�͂Ђӂւق��܂݂ނ߂�����Q��Q�患�����낡��Q���Q��\0" };//�܏\���̂Ђ炪�Ȕz��
	char word2[160] = { "�����������[�����������������������傾���Âłǂ�Ȃɂʂ˂̂�΂тԂׂڂ��܂݂ނ߂�����Q��Q�患�����낡��Q���Q��\0" };//�����Ђ炪�Ȕz��
	char word3[160] = { "111111111111111111111111111111111111111111111111111111111111�ς҂Ղ؂�11111111111111111111111111111111111111111111111111\0" };//�j�􉹂Ђ炪�Ȕz��
	char word4[160] = { "�A�C�E�G�I�[�J�L�N�P�R�b�T�V�X�Z�\���^�`�c�e�g���i�j�k�l�m���n�q�t�w�z�H�}�~�������F���Q���Q���D�����������B���Q���Q���@\0" };//�܏\���̂��Ȕz��
	char word5[160] = { "�A�C���G�I�[�K�M�O�Q�S�b�U�W�Y�[�]���_�a�d�f�h���i�j�k�l�m���o�r�u�x�{�H�}�~�������F���Q���Q���D�����������B���Q���Q���@\0" };//�������Ȕz��
	char word6[160] = { "111111111111111111111111111111111111111111111111111111111111�p�s�v�y�|11111111111111111111111111111111111111111111111111\0" };//�j�􉹂��Ȕz��
	char word7[160] = { "�`�a�b�c�d�e�f�g�h�i�j�k�l�m�n�o�p�q�r�s�t�u�v�w�x�y�����󁔂����������������������������������������������������[�~���{\0" };//english�z��
	char word8[160] = { "�`�a�b�c�d�e�f�g�h�i�j�k�l�m�n�o�p�q�r�s�t�u�v�w�x�y�����󁔂����������������������������������������������������[�~���{\0" };//
	char word9[160] = { "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111\0" };//
	char *wordA = word1;
	char *wordB = word2;
	char *wordC = word3;
	
	while (ScreenFlip() == 0 && ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0 && edfl == 0){
		
		int RefreshTime = GetNowCount();//���ݎ��Ԏ擾

		if(langsel==0 || langsel==1){
			maxR=maxR1;//�J�[�\���ő�ʒu
			maxL=maxL1;//�J�[�\���ő�ʒu
			maxU=maxU1;//�J�[�\���ő�ʒu
			maxD=maxD1;//�J�[�\���ő�ʒu
			
		}else if(langsel==2){
			//maxR=maxR2;//�J�[�\���ő�ʒu
			//maxL=maxL2;//�J�[�\���ő�ʒu
			//maxU=maxU2;//�J�[�\���ő�ʒu
			//maxD=maxD2;//�J�[�\���ő�ʒu
			maxR=maxR1;//�J�[�\���ő�ʒu
			maxL=maxL1;//�J�[�\���ő�ʒu
			maxU=maxU1;//�J�[�\���ő�ʒu
			maxD=maxD1;//�J�[�\���ő�ʒu
		}
		///�ݒ�
		ClearDrawScreen();// ��ʂ�������
		DrawGraph(0, 0, GH7, TRUE);//�w�i
		DrawGraph(0, 240, GH1, TRUE);//�L�[�{�[�h�w�i�̔�
		DrawGraph(setx, sety, GH2, TRUE);//�J�[�\���\��
	    
	//lang�ɂ���Ďg���L�[�{�[�h�ƃJ�[�\���ғ��͈͂̑I��	
		if(langsel==0){ 
			//�J�[�\���ړ���O
			if (setx == maxR){

				if (sety == UD * 2 + maxU){ //�㉺�Z�b�g
					sety -= UD;
					count -= 2;
				}
				else if (sety == UD * 4 + maxU){ //���E�Z�b�g
					sety -= UD;
					count -= 2;
				}

			//�J�[�\���I��
				if (sety == UD * 3 + maxU){ 
					DrawGraph(setx, sety, GH4, TRUE);
				}
				else if (sety == UD + maxU){

					DrawGraph(setx, sety, GH5, TRUE);
				}
			}
			//�L�[�{�[�h�Z�b�g
			DrawGraph(0, 240, GH3, TRUE);
		}else if(langsel==1){
						//�J�[�\������
			if (setx == maxR){

				if (sety == UD * 2 + maxU){ //�㉺�Z�b�g
					sety -= UD;
					count -= 2;
				}
				else if (sety == UD * 4 + maxU){ //���E�Z�b�g
					sety -= UD;
					count -= 2;
				}

			//�J�[�\���I��
				if (sety == UD * 3 + maxU){ 
					DrawGraph(setx, sety, GH4, TRUE);
				}
				else if (sety == UD + maxU){

					DrawGraph(setx, sety, GH5, TRUE);
				}
			}
			//�L�[�{�[�h�Z�b�g
			DrawGraph(0, 240, GH8, TRUE);
		}else{
			//�J�[�\������
			if (setx == maxR){

				if (sety == UD * 2 + maxU){ //�㉺�Z�b�g
					sety -= UD;
					count -= 2;
				}
				else if (sety == UD * 4 + maxU){ //���E�Z�b�g
					sety -= UD;
					count -= 2;
				}

				//�J�[�\���I��
				if (sety == UD * 3 + maxU){ 
					DrawGraph(setx, sety, GH4, TRUE);
				}
				else if (sety == UD + maxU){

					DrawGraph(setx, sety, GH5, TRUE);
				}
			}
			//�L�[�{�[�h�Z�b�g
			DrawGraph(0, 240, GH9, TRUE);
		}

		// ������̕`��
		DrawGraph(160, 145, GH6, TRUE);
		if(loopcount==35){
			loopcount=0;
		}else{
			DrawGraph(180+(n/2*17), 155, GH10, TRUE);//
		}
		DrawString(180, 160, key, GetColor(255, 255, 255));
		loopcount++;

		//////////////////////////////////////////�ړ��R�}���h///////////////////////////////////////////////////////////


		if (CheckHitKey(KEY_INPUT_LEFT) && setx>maxL){
				
			if (w % 10 == 0){

				count += 12;

				setx -= RL;
			}
			w++;
		}else if(CheckHitKey(KEY_INPUT_RIGHT) && setx<maxR){
			
			if (w % 10 == 0){
				setx += RL;

				count -= 12;
			}
			w++;

		}else if (CheckHitKey(KEY_INPUT_DOWN) && sety<maxD){

			if (w % 10 == 0){
				if ((sety == UD * 3 + maxU || sety == UD + maxU) && setx == maxR){
					count += 4;
					sety += UD * 2;
				}
				else{
					count += 2;
					sety += UD;
				}
			}
			w++;

		}else if (CheckHitKey(KEY_INPUT_UP) && sety>maxU){
			if (w % 10 == 0){
				
				if ((sety == UD * 5 + maxU || sety == UD * 3 + maxU) && setx == maxR){
					count -= 4;
					sety -= UD * 2;
				}
				else{
					count -= 2;
					sety -= UD;
					
				}
			}
			w++;
		}
/////////////////����R�}���h////////////////////////////////////////
		else if (CheckHitKey(KEY_INPUT_X)) {

				if (w % 10 == 0){
					if (setx == maxR){
						if (sety == UD + maxU){
							//��������
							if(n>0){
								n -= 2;
								key[n]           =	'\0';
								key[n + 1]       =	'\0';
								lastcount[n]     =	'\0';
								lastcount[n + 1] =	'\0';
								lastlang[n]      =    0 ;
								lastcount2[n]	 =    0 ;
							}

						}
						else if (sety == UD * 3 + maxU){
							//����{�^��
							edfl = 1;
						}
						else if (sety == UD * 5 + maxU){
							n -= 2;
							key[n] = '\0';
							key[n + 1] = '\0';
							if(lastlang[n]==0){
								wordA = word1;
								wordB = word2;
								wordC = word3;
							}else if(lastlang[n]==1){
								wordA = word4;
								wordB = word5;
								wordC = word6;
							}else if(lastlang[n]==2){
								wordA = word7;
								wordB = word8;
								wordC = word9;
							}

							if (lastcount2[n] == 1){
								if (wordC[lastcount[n]] == '1'){
									key[n] = wordA[lastcount[n]];
									key[n + 1] = wordA[lastcount[n + 1]];
									lastcount2[n] = 0;
								}else{
									key[n] = wordC[lastcount[n]];
									key[n + 1] = wordC[lastcount[n + 1]];
									lastcount2[n] = 2;
								}
							}else if (lastcount2[n] == 2){
								key[n] = wordA[lastcount[n]];
								key[n + 1] = wordA[lastcount[n + 1]];
								lastcount2[n] = 0;
							}
							else{
								key[n] = wordB[lastcount[n]];
								key[n + 1] = wordB[lastcount[n + 1]];
								lastcount2[n] = 1;
							}
							n += 2;

						}
						else if(sety == maxU){
							langsel += 1;
							if(langsel==3){
								langsel=0;
							}
							if(langsel==0){
								wordA = word1;
								wordB = word2;
								wordC = word3;
							}else if(langsel==1){
								wordA = word4;
								wordB = word5;
								wordC = word6;
							}else if(langsel==2){
								wordA = word7;
								wordB = word8;
								wordC = word9;
							}

						}
					}
					else if (n<max * 2){

						lastlang[n] = langsel;

						//��������
						key[n] = wordA[count];
						key[n + 1] = wordA[count + 1];
						//�����X�V
						lastcount[n]     = count;
						lastcount[n + 1] = count + 1;
						lastlang[n]      = langsel  ;
						n += 2;
					}

				}
				w++;

			}
			else {
				//���������Z�b�g
				w = 0;

				

			}











		

		while (GetNowCount() - RefreshTime < 17 && ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0);

	}

	ClearDrawScreen();

	name2 = key;
	return name2;

};