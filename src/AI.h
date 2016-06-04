#pragma once

#include "field.h"
#include "move.h"
#include "search.h"

enum NodeType { ROOT, PV, NO_PV, SPLITPOINT_ROOT, SPLITPOINT_PV, SPLITPOINT_NO_PV };

// 基本となるAIクラス
class AI
{
public:
	AI() {};
	AI(int d) :depth_max_recieve_(d) {};

	void setLevel(int depth) { depth_max_recieve_ = depth; }
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

protected:

	// 読みの深さ
	int depth_max_;

	// レベル
	int level_;

	// ユーザからもらった値。この深さで探索する。
	int depth_max_recieve_;

	Move best_[10];

	std::vector<RootMove> root_moves;
	Operate operate_;
	int continue_self_num_;
	unsigned char easy_;
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
				int save = depth_max_recieve_;
				depth_max_recieve_ = 6;
				int score = thinkWrapper(self, enemy);
				depth_max_recieve_ = save;
				return score;
			}
			else if (self.getPuyoNum() < 12)
			{
				int save = depth_max_recieve_;
				depth_max_recieve_ = 4;
				int score = thinkWrapper(self, enemy);
				depth_max_recieve_ = save;
				return score;
			}
			else
			{
				return thinkWrapperEX(self, enemy);
			}
		}
	};
};