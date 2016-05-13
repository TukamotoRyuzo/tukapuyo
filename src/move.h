#pragma once
#include "platform.h"
#include "puyo.h"
#include <queue>

class LightField;
class Field;

class Move
{

public:
	Move(){};

	explicit Move(uint16_t t):m_(t){};

	uint32_t operator=(const Move& m){return m_ = m.get();}
	uint32_t operator=(uint16_t m){return m_ = m;}

	// getter
	uint32_t get() const { return m_; }

	// moveは16ビットにパックする
	Move(const Square psquare, const Square csquare, const bool tigiri) { m_ = tigiri << 15 | csquare << 8 | psquare; }


	bool isLegal(LightField f);
	bool isLegalAbout(const LightField& f) const ;

	bool operator == (const Move m) const { return m_ == m.get(); }
	bool operator != (const Move m) const { return m_ != m.get(); }

	Square psq() const { return static_cast<Square>(m_ & 0x007f); }
	Square csq() const { return static_cast<Square>((m_ & 0x7f00) >> 8); }
	bool isTigiri() const { return  static_cast<bool>(m_ & 0x8000); }
	bool isNone() const { return m_ == 0; }
	std::string toString(LightField f, const int depth) const;
	static Move moveNone() { return Move(0); }

private:
	uint16_t m_;
};

struct MoveStack
{
	int32_t score;
	Move move;
};

struct Operate
{
	std::queue<int> queue_;

	void clear() { while (!queue_.empty()) queue_.pop(); }
	int pop() { int ret = queue_.front(); queue_.pop(); return ret; }
	void push(int o) { queue_.push(o); }
	void generate(const Move move, const Field &f);
	void easyGenerate(const Move move, const Field &f);
	void veryEasyGenerate(const Move move, const Field &f);
};