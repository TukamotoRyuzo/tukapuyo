#include "dxlib.h"
#include "iostream"
#include "gamemode.h"
std::string nameboad(){

	
	const int RL = 32;//カーソル移動right,left

	const int UD = 32;//カーソル移動up,down

	int setx = 288;//カーソルx値(初期位置:あ-＞288)
	const int startx = setx;

	int sety = 244;//カーソルy値(初期位置:あ-＞244)
	const int starty = sety;
	//6×11
	const int maxR1 = setx+RL;//カーソル最大位置
	const int maxL1 = setx-RL*9;//カーソル最大位置
	const int maxU1 = sety;//カーソル最大位置
	const int maxD1 = sety+UD*5;//カーソル最大位置


	int maxR;//カーソル最大位置
	int maxL;//カーソル最大位置
	int maxU;//カーソル最大位置
	int maxD;//カーソル最大位置






	std::string name2="";//return値であるstring型文字配列(選択した名前)
	char key[120]="";//選択した文字を適宜表示するためのchar型文字
	
	int count = 0;//五十音表からひらがなの選択

	int n = 0;//文字表示の配列内の文字数

	int w = 0;//ボタンの長押し調整

	int edfl = 0;//終了フラグ

	int lastcount[20];//文字を打ち込んだ時のcountの履歴

	int lastlang[20];//打ち込んだ文字の言語の履歴

	int lastcount2[20];//濁音フラグ

	int langsel = 0;//0:ひらがな 1:カタカナ 2:english

	int max = 10;//最大文字数

	int loopcount=0;

	//画像
	const int GH1 = LoadGraph( "picture\\backboad.png" );//背景の白
	const int GH2 = LoadGraph( "picture\\yerrow.png" );//カーソル表示
	const int GH3 = LoadGraph( "picture\\keyboad-透過ver3.png" );//ひらがなキーボード
	const int GH4 = LoadGraph( "picture\\enter_yerrow.png" );//カーソル表示 
	const int GH5 = LoadGraph( "picture\\return_yerrow.png" );//カーソル表示 
	const int GH6 = LoadGraph( "picture\\ggg.png" ) ;//名前表示 
	const int GH7 = LoadGraph( "picture\\背景_vertest.png" ) ;//背景表示
	const int GH8 = LoadGraph( "picture\\keyboad-kana.png" );//カタカナキーボード
	const int GH9 = LoadGraph( "picture\\keyboad-english.png" );//englishキーボード
	const int GH10= LoadGraph( "picture\\wordline.png" ) ;//文字ライン表示


	char word1[160] = { "あいうえおーかきくけこっさしすせそょたちつてとゅなにぬねのゃはひふへほぉまみむめもぇや＿ゆ＿よぅらりるれろぃわ＿を＿んぁ\0" };//五十音のひらがな配列
	char word2[160] = { "あいうえおーがぎぐげごっざじずぜぞょだぢづでどゅなにぬねのゃばびぶべぼぉまみむめもぇや＿ゆ＿よぅらりるれろぃわ＿を＿んぁ\0" };//濁音ひらがな配列
	char word3[160] = { "111111111111111111111111111111111111111111111111111111111111ぱぴぷぺぽ11111111111111111111111111111111111111111111111111\0" };//破裂音ひらがな配列
	char word4[160] = { "アイウエオーカキクケコッサシスセソョタチツテトュナニヌネノャハヒフヘホォマミムメモェヤ＿ユ＿ヨゥラリルレロィワ＿ヲ＿ンァ\0" };//五十音のかな配列
	char word5[160] = { "アイヴエオーガギグゲゴッザジズゼゾョダヂヅデドュナニヌネノャバビブベボォマミムメモェヤ＿ユ＿ヨゥラリルレロィワ＿ヲ＿ンァ\0" };//濁音かな配列
	char word6[160] = { "111111111111111111111111111111111111111111111111111111111111パピプペポ11111111111111111111111111111111111111111111111111\0" };//破裂音かな配列
	char word7[160] = { "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ＆＊♭＃ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚー×÷＋\0" };//english配列
	char word8[160] = { "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ＆＊♭＃ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚー×÷＋\0" };//
	char word9[160] = { "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111\0" };//
	char *wordA = word1;
	char *wordB = word2;
	char *wordC = word3;
	
	while (ScreenFlip() == 0 && ProcessMessage() == 0 && ClearDrawScreen() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0 && edfl == 0){
		
		int RefreshTime = GetNowCount();//現在時間取得

		if(langsel==0 || langsel==1){
			maxR=maxR1;//カーソル最大位置
			maxL=maxL1;//カーソル最大位置
			maxU=maxU1;//カーソル最大位置
			maxD=maxD1;//カーソル最大位置
			
		}else if(langsel==2){
			//maxR=maxR2;//カーソル最大位置
			//maxL=maxL2;//カーソル最大位置
			//maxU=maxU2;//カーソル最大位置
			//maxD=maxD2;//カーソル最大位置
			maxR=maxR1;//カーソル最大位置
			maxL=maxL1;//カーソル最大位置
			maxU=maxU1;//カーソル最大位置
			maxD=maxD1;//カーソル最大位置
		}
		///設定
		ClearDrawScreen();// 画面を初期化
		DrawGraph(0, 0, GH7, TRUE);//背景
		DrawGraph(0, 240, GH1, TRUE);//キーボード背景の白
		DrawGraph(setx, sety, GH2, TRUE);//カーソル表示
	    
	//langによって使うキーボードとカーソル稼動範囲の選択	
		if(langsel==0){ 
			//カーソル移動例外
			if (setx == maxR){

				if (sety == UD * 2 + maxU){ //上下セット
					sety -= UD;
					count -= 2;
				}
				else if (sety == UD * 4 + maxU){ //左右セット
					sety -= UD;
					count -= 2;
				}

			//カーソル選定
				if (sety == UD * 3 + maxU){ 
					DrawGraph(setx, sety, GH4, TRUE);
				}
				else if (sety == UD + maxU){

					DrawGraph(setx, sety, GH5, TRUE);
				}
			}
			//キーボードセット
			DrawGraph(0, 240, GH3, TRUE);
		}else if(langsel==1){
						//カーソル限定
			if (setx == maxR){

				if (sety == UD * 2 + maxU){ //上下セット
					sety -= UD;
					count -= 2;
				}
				else if (sety == UD * 4 + maxU){ //左右セット
					sety -= UD;
					count -= 2;
				}

			//カーソル選定
				if (sety == UD * 3 + maxU){ 
					DrawGraph(setx, sety, GH4, TRUE);
				}
				else if (sety == UD + maxU){

					DrawGraph(setx, sety, GH5, TRUE);
				}
			}
			//キーボードセット
			DrawGraph(0, 240, GH8, TRUE);
		}else{
			//カーソル限定
			if (setx == maxR){

				if (sety == UD * 2 + maxU){ //上下セット
					sety -= UD;
					count -= 2;
				}
				else if (sety == UD * 4 + maxU){ //左右セット
					sety -= UD;
					count -= 2;
				}

				//カーソル選定
				if (sety == UD * 3 + maxU){ 
					DrawGraph(setx, sety, GH4, TRUE);
				}
				else if (sety == UD + maxU){

					DrawGraph(setx, sety, GH5, TRUE);
				}
			}
			//キーボードセット
			DrawGraph(0, 240, GH9, TRUE);
		}

		// 文字列の描画
		DrawGraph(160, 145, GH6, TRUE);
		if(loopcount==35){
			loopcount=0;
		}else{
			DrawGraph(180+(n/2*17), 155, GH10, TRUE);//
		}
		DrawString(180, 160, key, GetColor(255, 255, 255));
		loopcount++;

		//////////////////////////////////////////移動コマンド///////////////////////////////////////////////////////////


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
/////////////////決定コマンド////////////////////////////////////////
		else if (CheckHitKey(KEY_INPUT_X)) {

				if (w % 10 == 0){
					if (setx == maxR){
						if (sety == UD + maxU){
							//文字消去
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
							//決定ボタン
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

						//文字入力
						key[n] = wordA[count];
						key[n + 1] = wordA[count + 1];
						//履歴更新
						lastcount[n]     = count;
						lastcount[n + 1] = count + 1;
						lastlang[n]      = langsel  ;
						n += 2;
					}

				}
				w++;

			}
			else {
				//長押しリセット
				w = 0;

				

			}











		

		while (GetNowCount() - RefreshTime < 17 && ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0);

	}

	ClearDrawScreen();

	name2 = key;
	return name2;

};