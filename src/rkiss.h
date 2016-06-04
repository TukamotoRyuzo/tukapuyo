#pragma once
#include"platform.h"

// RKissはhash keyを計算するために使う擬似乱数発生器(PRNG)である。
// George Marsaglは、90年代はじめにRNG-Kiss-familyを発明した。
// これはBob Jenkinsのpublic domainのコードからHeinz van Saanenが派生させた
// 特殊化バージョンである。Heinzによりテストされたように、以下の特徴がある。
// // / - きわめてプラットフォームに依存しない
// / - すべてのdie harderテストをパスした！ *nix(※　Unixを伏せ字にしてあるのか？)のsys-rand()なんかは悲惨な結果になる。
// / - *nixのsys-rand()より12倍ぐらい速い。
// / - SSE2-version of Mersenne twisterのSSE2版より4倍ぐらい速い
// / - 平均周期 : 2^126ぐらい
// / - 64 bitの乱数シード
// / - フル53 bitの仮数からなるdouble型を返せる(※　暗黙の型変換子が書いてあるので何型でも返せる)
// / - スレッドセーフ 


class RKiss 
{   
	uint64_t a, b, c, d;

	uint64_t rotate(uint64_t x, uint64_t k) const 
	{    
		return (x << k) | (x >> (64 - k));  
	}   

	uint64_t rand64() 
	{     
		const uint64_t e = a - rotate(b, 7);
		a = b ^ rotate(c, 13);
		b = c + rotate(d, 37);
		c = d + e;
		return d = e + a;  
	} 
public:  
	// コンストラクタ。  
	// 乱数シードを変更して使うなら変更して使ってもいいが…。  
	RKiss(int seed = 73) 
	{     
		a = 0xF1EA5EED, b = c = d = 0xD4E12C77;

		for (int i = 0; i < seed; ++i)// Scramble a few rounds   
		{ 
			rand64();  
		}
	}   
	// 生成した乱数を取り出すための暗黙の型変換子。  
	// int64_t以外にもdoubleなどにも代入できる。  
	template<typename T> T rand() { return T(rand64()); }

}; 