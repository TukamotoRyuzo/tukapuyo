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

	// b��true�ɂȂ�܂ő҂�
	void wait(std::atomic_bool& b);

	// b��true�̊ԑ҂�
	void waitWhile(std::atomic_bool& b);

	size_t threadId() const { return idx; }

	bool isMain() const { return idx == 0; }

	// ���̃X���b�h�ɑ債�ĒT�����J�n������Ƃ��͂�����Ăяo���B
	void startSearching(bool resume = false);

	// ���̃X���b�h��searching�t���O��false�ɂȂ�̂�҂B(MainThread��slave�̒T�����I������̂�ҋ@����̂Ɏg��)
	void join() { waitWhile(searching); }

	//Board root_board;
	//std::vector<Search::RootMove> root_moves;
	//Depth root_depth, completed_depth;

	// calls_count : ���̕ϐ���++�����񐔂�checkTime()���Ăяo�����ǂ����𔻒肷��.
	int max_ply, calls_count;

	// checkTime()���Ăяo���̂���߂邩�ǂ����̃t���O
	std::atomic_bool reset_calls;

	// beta cutoff�����w����ɉ��_���āA����ȊO��Quiet�Ȏ�ɂ͌��_�������́B
	//HistoryStats history;

	// ����w����ɑ΂���w�����ۑ����Ă����z��
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

struct Slaves // MainThread���������[�v���܂킷���߁B
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