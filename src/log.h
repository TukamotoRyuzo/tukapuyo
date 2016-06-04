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

	// �t�@�C���X�g���[���ւ̏o�̓I�y���[�^
	// �o�͂���ƒ��g���������Ⴄ
	friend std::ofstream& operator<<(std::ofstream&, History&);
	friend std::ifstream& operator>>(std::ifstream&, History&);

private:
	std::queue<int> history_;
};

