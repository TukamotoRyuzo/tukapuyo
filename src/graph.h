#pragma once
#include"DxLib.h"
#include"vector.h"

// 画像に関するクラス
class Graph
{
public:
	Graph(){};
	Graph(int handle, bool flag);// 画像ハンドルからインスタンスを作る。
	void init(int handle, bool flag);// コンストラクタと同じ
	void load(const TCHAR *FileName , bool flag);// ロード
	
	void setPosition(Vector pos);// 座標の設定(Vectorバージョン)
	void setPosition(int x , int y);// 座標の設定（各数値指定バージョン）

	void draw(float BlendParam = 100.0f);// 描画
	int handle() const ;// 画像ハンドルの取得
	Vector getUpperLeft() const ;// 左上の座標を取得
	Vector getLowerRight() const ;// 右下の座標を取得
	Vector size() const ;// 画像のサイズを取得
private:
	int handle_;// 画像ハンドル
	Vector pos_;// 座標
	Vector size_;// サイズ
	bool flag_;// 透過させるかのフラグ
};

