#include"graph.h"

// コンストラクタ: ハンドルからインスタンスを作る
Graph::Graph(int handle, bool flag)
{
	init(handle, flag);	
}

// コンストラクタ以外からでも初期化できるように。
void Graph::init(int handle, bool flag)
{
	handle_ = handle;

	// 画像のサイズ取得
	int x , y;
	DxLib::GetGraphSize(handle_ , &x , &y);
	size_.x = x; size_.y = y;
	
	flag_ = flag;
}

// 画像のロード
void Graph::load(const TCHAR *FileName , bool flag)
{
	// 画像のロード
	handle_ = DxLib::LoadGraph(FileName);

	if (handle_ == -1)
	{
		//printfDx("DAME");
	}
	else
	{
		//printfDx("OK");
	}
	// 画像のサイズ取得
	int x, y;
	DxLib::GetGraphSize(handle_ , &x , &y);
	size_.x = x;
	size_.y = y;

	// 透過させるかのフラグの設定
	flag_ = flag;

}


// 画像の座標を設定（Vectorバージョン）
void Graph::setPosition(Vector pos)
{
	pos_.x = pos.x;
	pos_.y = pos.y;
}

// 画像の座標を設定（各数値指定バージョン）
void Graph::setPosition(int x , int y)
{
	pos_.x = x; pos_.y = y;
}

// 画像の描画
void Graph::draw(float BlendParam)
{
	DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA , (int)(255 * BlendParam));
	DxLib::DrawGraph((int)pos_.x , (int)pos_.y , handle_ , flag_);
	DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND , 0);
}

// 画像ハンドルの取得
int Graph::handle() const 
{
	return handle_;
}

// 画像の左上の座標を取得
Vector Graph::getUpperLeft() const  
{
	return pos_;
}

// 画像の右下の座標を取得
Vector Graph::getLowerRight() const 
{
	Vector calc;
	calc.x = pos_.x + size_.x;
	calc.y = pos_.y + size_.y;

	return calc;
}

// 画像のサイズを取得
Vector Graph::size() const 
{
	return size_;
}