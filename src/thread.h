#pragma once

#include "platform.h"
#include "field.h"
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>

const int MAX_THREADS = 64;
typedef std::mutex Mutex;
typedef std::condition_variable ConditionVariable;

struct Thread
{
	Thread();
	virtual ~Thread();
	//virtual void search();
	void idleLoop();

	// bがtrueになるまで待つ
	void wait(std::atomic_bool& b);

	// bがtrueの間待つ
	void waitWhile(std::atomic_bool& b);

	size_t threadId() const { return idx; }

	bool isMain() const { return idx == 0; }

	// このスレッドに大して探索を開始させるときはこれを呼び出す。
	void startSearching(bool resume = false);

	// このスレッドのsearchingフラグがfalseになるのを待つ。(MainThreadがslaveの探索が終了するのを待機するのに使う)
	void join() { waitWhile(searching); }

	//Board root_board;
	//std::vector<Search::RootMove> root_moves;
	//Depth root_depth, completed_depth;

	// calls_count : この変数を++した回数でcheckTime()を呼び出すかどうかを判定する.
	int max_ply, calls_count;

	// checkTime()を呼び出すのをやめるかどうかのフラグ
	std::atomic_bool reset_calls;

	// beta cutoffした指し手に加点して、それ以外のQuietな手には減点したもの。
	//HistoryStats history;

	// ある指し手に対する指し手を保存しておく配列
	//MoveStats counter_moves;

protected:
	std::thread native_thread;
	ConditionVariable sleep_condition;
	Mutex mutex;
	std::atomic_bool exit, searching;
	size_t idx;
};

struct MainThread : public Thread
{
	//virtual void search();

	bool failed_low;
	double best_move_changes;
	Score previous_score;
};

struct Slaves // MainThreadを除くループをまわすため。
{
	std::vector<Thread*>::iterator begin() const;
	std::vector<Thread*>::iterator end() const;
};

struct ThreadPool : public std::vector<Thread*>
{
	void init();
	void exit();
	MainThread* main() { return static_cast<MainThread*>(at(0)); }
	//void startThinking(const Board& b, const LimitsType& limits);
	//int64_t nodeSearched() { int64_t nodes = 0; for (auto* th : *this) nodes += th->root_board.nodeSearched(); return nodes; }
	Slaves slaves;
	//void readUsiOptions();
};

extern ThreadPool Threads;
enum SyncCout { IO_LOCK, IO_UNLOCK };

inline std::ostream& operator << (std::ostream& os, SyncCout sc)
{
	static std::mutex m;
	if (sc == IO_LOCK) { m.lock(); }
	if (sc == IO_UNLOCK) { m.unlock(); }
	return os;
}

#define SYNC_COUT std::cout << IO_LOCK
#define SYNC_ENDL std::endl << IO_UNLOCK