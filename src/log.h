#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <cassert>

class History
{
public:
	History(){};
	
	void push(int i){ history_.push(i); }
	int pop() { assert(!history_.empty());int ret = history_.front(); history_.pop(); return ret;}
	void clear() { while(!history_.empty())pop(); }

	// ファイルストリームへの出力オペレータ
	// 出力すると中身が消えちゃう
	friend std::ofstream& operator<<(std::ofstream&, History&);
	friend std::ifstream& operator>>(std::ifstream&, History&);

private:
	std::queue<int> history_;
};

