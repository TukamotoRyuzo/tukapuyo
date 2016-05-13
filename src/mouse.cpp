#include"mouse.h"


// 時間経過で変化する変数の設定
void Mouse::SetPermanent(){

    // 前フレームの座標を取得
    before = after;

    // マウスの座標を取得
    int x , y;
    GetMousePoint( &x , &y );
    pos.x = x; pos.y = y;
   

    // 今の座標を取得
    after = pos;

    // ボタンが押されていたら増加　ボタンを離すと0になる
    if( ( GetMouseInput() & MOUSE_INPUT_LEFT ) != 0 )button[0]++;
    else button[0] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_RIGHT ) !=0 )button[1]++;
    else button[1] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_MIDDLE ) !=0 )button[2]++;
    else button[2] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_4 ) !=0 )button[3]++;
    else button[3] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_5 ) !=0 )button[4]++;
    else button[4] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_6 ) !=0 )button[5]++;
    else button[5] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_7 ) !=0 )button[6]++;
    else button[6] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_8 ) !=0 )button[7]++;
    else button[7] = 0;

    // ホイールの回転量を調べる
    int rot = GetMouseWheelRotVol();

    if( rot < 0 )wheel--;
    else if( 0 < rot )wheel++;
    else wheel = 0;

}

// マウスカーソルの表示設定フラグのセット
void Mouse::SetDispFlag( bool DispFlag ){
    SetMouseDispFlag( DispFlag );
}

// マウスカーソルの位置を取得する
Vector Mouse::GetPoint(){
    return pos;
}

// マウスカーソルの位置をセットする
void Mouse::SetPoint( Vector pos ){
    
    this->pos = pos;

    DxLib::SetMousePoint( (int)this->pos.x , (int)this->pos.y );
}

// マウスカーソルの位置をセットする
void Mouse::SetPoint( int x , int y ){
    
    this->pos.x = x;
    this->pos.y = y;

    DxLib::SetMousePoint( (int)this->pos.x , (int)this->pos.y );
}

// マウスカーソルのボタンを押している時間を得る
int Mouse::GetInput( int num )
{
    // エラー処理
    if( num < 0 || 7 < num ){
        printfDx( "不明なマウスボタンの設定\n" );
        return -1;
    }

    return button[ num ];
}

// マウスホイールの回転量を得る
int Mouse::GetWheelRotVol(){
    return wheel;
}

// 画像との当たり判定
bool Mouse::JudgeCollisionGraph( Graph graph ){

    // マウスの座標が画像の左端よりも右
    if( graph.getUpperLeft().x < this->pos.x )
		
        // マウスの座標が画像の右端よりも左
        if( this->pos.x < graph.getLowerRight().x )

            // マウスの座標が画像の上端よりも下
            if( graph.getUpperLeft().y < this->pos.y )

                // マウスの座標が画像の下端よりも上
                if( this->pos.y < graph.getLowerRight().y )

                    return TRUE;

    // 条件に合わなければFALSE
    return FALSE;

}

// マウスが動いているか判定
bool Mouse::JudgeMouseMove(){

    // 動いていたらTRUE
    if( this->before.x != this->after.x || this->before.y != this->after.y )
        return TRUE;
    else return FALSE;

}
