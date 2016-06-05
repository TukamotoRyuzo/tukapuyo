#include "thread.h"

ThreadPool Threads;

Thread::Thread()
{
	// �����o������
	reset_calls = exit = false;
	max_ply = calls_count = 0;
	//history.clear();
	//counter_moves.clear();
	idx = Threads.size();

	// �X���b�h��idleLoop����sleep����܂ł𐳏�Ɏ��s�����鏈��
	std::unique_lock<Mutex> lk(mutex);
	searching = true;

	// ���̑�����I����ƃX���b�h��idleLoop�����s���o��
	native_thread = std::thread(&Thread::idleLoop, this);

	// Thread::idleLoop�ŃX�^���o�COK�ɂȂ�܂ő҂�
	sleep_condition.wait(lk, [&] { return !searching; });
}

Thread::~Thread()
{
	mutex.lock();
	exit = true;

	// exit��true�ɂ������Ƃ�ʒm���AidleLoop���I����҂B
	sleep_condition.notify_one();
	mutex.unlock();

	// �T���I����҂B
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

// ���̃X���b�h�ɑ債�ĒT�����J�n������Ƃ��͂�����Ăяo���B
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
			// �������ł������Ƃ�ʒm����BThread()�ƁAMainThread::search()�őҋ@���Ă���X���b�h�ɑ΂��čs��
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

// ThreadPool�̒���main�X���b�h�Aslave�X���b�h���T�����J�n���邽�߂̏�����������B
//void ThreadPool::startThinking(const Board& b, const LimitsType& limits)
//{
//	// ���C���X���b�h�̎Q����҂B
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


