#pragma once
#include"DxLib.h"
#include"vector.h"
#include"graph.h"

enum MouseID
{
	MOUSE_LEFT, MOUSE_RIGHT, MOUSE_CENTOR,
};

// マウスに関するクラス
class Mouse{
private:
    Vector pos;// 座標
    int button[8];// ボタンを押している時間
    int wheel;// ホイール
    Vector before , after;// マウスが動いたか判定

public:
	
    void SetPermanent();// 時間経過で変化する変数の設定
    void SetDispFlag( bool DispFlag );// マウスカーソルの表示設定フラグのセット
    void SetPoint( Vector pos );// マウスカーソルの位置をセットする
    void SetPoint( int x , int y );// マウスカーソルの位置をセットする

    Vector GetPoint();// マウスカーソルの位置を取得する
    int GetInput(int num);// マウスカーソルのボタンを押している時間を得る
    int GetWheelRotVol();// マウスホイールの回転量を得る
    bool JudgeCollisionGraph(Graph graph);// 画像との当たり判定
    bool JudgeMouseMove();// マウスが動いたかどうか判定

};
