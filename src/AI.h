#pragma once

#include <atomic>
#include <thread>
#include "field.h"
#include "move.h"
#include "search.h"

enum NodeType;
// 基本となるAIクラス
class AI
{
public:
	AI() {};
	AI(int d) :depth_max_(d) {};

	void setLevel(int depth) { depth_max_ = depth; }
	int getLevel() const { return depth_max_; }
	Operate* operate() { return &operate_; }

	int thinkWrapper(const Field &self, const Field &enemy);

	int think(LightField &self, LightField &enemy, int depth, int timeLimit);// max探索

	virtual int evalNoVanish(const LightField &self, const LightField &enemy, int depth, int timeLimit) const { return 0; }
	virtual int evalVanish(LightField self, const LightField &enemy, int depth, int timeLimit) { return 0; }

	int evalate(LightField &self, LightField &enemy, int depth, int timeLimit)
	{
		return self.flag(VANISH) ? evalVanish(self, enemy, depth, timeLimit)
			: evalNoVanish(self, enemy, depth, timeLimit);
	}

	// Polytec2015
	int thinkWrapperEX(Field self, Field enemy);

	template <NodeType NT>
	Score search(Score alpha, Score beta, LightField& self, LightField& enemy, int depth, int enemy_put_time);// 基本となるアルファベータ探索
	Score evaluateEX(LightField &self, LightField &enemy, int depth, int my_remain_time);
	void checkTime();
protected:

	// 読みの深さ
	int depth_max_;

	// レベル
	int level_;

	Move best_[DEPTH_MAX];

	std::vector<Search::RootMove> root_moves;
	Operate operate_;
	int continue_self_num_;
	unsigned char easy_;

	std::atomic_bool stop;
	int calls_count;
};

// 3つつなげるように努力する
// ここからはAIクラスで定義したthinkメソッドを使う
class AI3Connect : public AI
{
public:
	AI3Connect() {};
	AI3Connect(int d) : AI(d) {};
	virtual int evalNoVanish(const LightField &self, const LightField &enemy, int depth, int timeLimit) const;
	virtual int evalVanish(LightField self, const LightField &enemy, int depth, int timeLimit);
};

class AI3Connect2 : public AI
{
public:
	AI3Connect2() {};
	AI3Connect2(int d) : AI(d) {};
	virtual int evalNoVanish(const LightField &self, const LightField &enemy, int depth, int timeLimit) const;
	virtual int evalVanish(LightField self, const LightField &enemy, int depth, int timeLimit);
};

class PolytecAI : public AI3Connect2
{
public:
	PolytecAI() {};

	// レベル設定
	void setLevel(int level);
	int getLevel() const { return level_; }

	int levelThink(const Field self, const Field enemy)
	{
		if (level_ >= 1 && level_ <= 4)
		{
			return thinkWrapper(self, enemy);
		}
		else
		{
			if (enemy.flag(RENSA) && enemy.chainMax() > 7/* && self.getPuyoNum() < 45*/)// 敵が連鎖中
			{
				int save = depth_max_;
				depth_max_ = 6;
				int score = thinkWrapper(self, enemy);
				depth_max_ = save;
				return score;
			}
			else if (self.getPuyoNum() < 12)
			{
				int save = depth_max_;
				depth_max_ = 6;
				int score = thinkWrapper(self, enemy);
				depth_max_ = save;
				return score;
			}
			else
			{
				return thinkWrapperEX(self, enemy);
			}
		}
	};
};