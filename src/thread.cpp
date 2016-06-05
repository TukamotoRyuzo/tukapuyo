#include "thread.h"

ThreadPool Threads;

Thread::Thread()
{
	// メンバ初期化
	reset_calls = exit = false;
	max_ply = calls_count = 0;
	//history.clear();
	//counter_moves.clear();
	idx = Threads.size();

	// スレッドがidleLoop内でsleepするまでを正常に実行させる処理
	std::unique_lock<Mutex> lk(mutex);
	searching = true;

	// この操作を終えるとスレッドがidleLoopを実行し出す
	native_thread = std::thread(&Thread::idleLoop, this);

	// Thread::idleLoopでスタンバイOKになるまで待つ
	sleep_condition.wait(lk, [&] { return !searching; });
}

Thread::~Thread()
{
	mutex.lock();
	exit = true;

	// exitをtrueにしたことを通知し、idleLoopを終了を待つ。
	sleep_condition.notify_one();
	mutex.unlock();

	// 探索終了を待つ。
	native_thread.join();
}

void Thread::wait(std::atomic_bool& b)
{
	std::unique_lock<Mutex> lk(mutex);
	sleep_condition.wait(lk, [&] { return bool(b); });
}

void Thread::waitWhile(std::atomic_bool& b)
{
	std::unique_lock<Mutex> lk(mutex);
	sleep_condition.wait(lk, [&] { return !b; });
}

// このスレッドに大して探索を開始させるときはこれを呼び出す。
void Thread::startSearching(bool resume)
{
	std::unique_lock<Mutex>(mutex);

	if (!resume)
		searching = true;

	sleep_condition.notify_one();
}

void Thread::idleLoop()
{
	while (!exit)
	{
		std::unique_lock<Mutex> lk(mutex);

		searching = false;

		while (!searching && !exit)
		{
			// 準備ができたことを通知する。Thread()と、MainThread::search()で待機しているスレッドに対して行う
			sleep_condition.notify_one();
			sleep_condition.wait(lk); 
		}

		lk.unlock();

		//if (!exit)
		//	search();
	}
}

std::vector<Thread*>::iterator Slaves::begin() const { return Threads.begin() + 1; }
std::vector<Thread*>::iterator Slaves::end() const { return Threads.end(); }

void ThreadPool::init()
{
	push_back(new MainThread);
	//readUsiOptions();
}

void ThreadPool::exit()
{
	while (size())
		delete back(), pop_back();
}

// ThreadPoolの中のmainスレッド、slaveスレッドが探索を開始するための初期化をする。
//void ThreadPool::startThinking(const Board& b, const LimitsType& limits)
//{
//	// メインスレッドの参加を待つ。
//	main()->join();
//
//	USI::Signals.stop_on_ponderhit = USI::Signals.stop = false;
//
//	main()->root_moves.clear();
//	main()->root_board = b;
//	USI::Limits = limits;
//
//	for (const auto& m : MoveList<LEGAL>(b))
//		if (limits.search_moves.empty() || count(limits.search_moves.begin(), limits.search_moves.end(), m))
//			main()->root_moves.push_back(Search::RootMove(m));
//
//	main()->startSearching();
//}
//
//void ThreadPool::readUsiOptions()
//{
//	size_t requested = USI::Options["Threads"];
//
//	while (size() < requested)
//		push_back(new Thread);
//
//	while (size() > requested)
//		delete back(), pop_back();
//}


