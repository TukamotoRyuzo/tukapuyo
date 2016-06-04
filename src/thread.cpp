#include"thread.h"
#include<climits>


// スレッドプール。事前にスレッドを必要数分だけ起動させ、ストックしておく。
ThreadPool Threads; // Global object

namespace 
{
	// start_routine() is the C function which is called when a new thread
	// is launched. It is a wrapper to the virtual function idle_loop().
	// 新しいスレッドが起動されたときに呼ばれるCの関数
	// idle_loop()のラッパー関数である
	extern "C" { long StartRoutine(ThreadBase* th) { th->IdleLoop(); return 0; } }


	// Helpers to launch a thread after creation and joining before delete. Must be
	// outside Thread c'tor and d'tor because object shall be fully initialized
	// when start_routine (and hence virtual idle_loop) is called and when joining.

	// スレッド生成のあとスレッドを起動させ、スレッドがdeleteされる前にjoinするためのヘルパー群。
	// Threadクラスのコンストラクタおよびデストラクタの外側でなければならない。
	// なぜならidle_loop()仮想関数が呼び出されときや、joinしているときはオブジェクトは
	// 完全に初期化されていなければならないので。
	// ToDo : いまひとつ意味がわからない。

	// TはThreadBaseの派生クラスと仮定している。
	// new Tして、std::threadを生成し、idle_loopを呼び出す。このときに引数としてこのnewしたものを渡す。
	// また、この関数はnewしたものをリターンする。
	// Tとして渡しうるのは、
	// TimerThread : タイマースレッド
	// MainThread : 1つ目の思考スレッド
	// Thread : 2つ目以降の思考スレッド
	// である。
	template<typename T> T* NewThread()
	{
		T* th = new T();
		thread_create(th->handle, StartRoutine, th); // Will go to sleep
		return th;
	}

	// スレッドを停止させ、その終了を待つ。
	void DeleteThread(ThreadBase* th) 
	{
		// exitフラグをtrueにする。これは探索がすでに終了していなければならない。
		th->exit = true; // Search must be already finished
		th->NotifyOne();

		// ネイティブスレッドの終了を待つ。
		thread_join(th->handle); // wait for thread termination
		delete th;
	}
}//namespace

// ThreadBase::notify_one()は、することが何か探索にあるときにidle_loop()のなかで眠っているスレッドを起こすのに使う。
void ThreadBase::NotifyOne()
{
	mutex.lock();
	sleep_condition.notify_one();
	mutex.unlock();
}

// ThreadBase::wait_for ()は、引数で指定した変数bがtrueになるまでスレッドをsleepさせる。
// ※　RootPos.this_thread()->wait_for (Signals.stop);　のように使う。
void ThreadBase::WaitFor(volatile const bool& b) {

	mutex.lock();
	while (!b) sleep_condition.wait(mutex);
	mutex.unlock();
}

// TimerThread::idle_loop() は、タイマースレッドがmsecミリ秒を待ち、check_time()を呼び出す。
// もしmsecが0なら、threadが起こされるまで眠り続ける。
// ※ sleepConditionのシグナルで起こされるまで。
void TimerThread::IdleLoop()
{
	while (!exit)
	{
		mutex.lock();

		// sleepConditionがシグナル状態になるのを待つ。
		// タイムアウト条件は、runが0ならINT_MAX(無限に)、そうでなければResolutionだけ。

		// runは仕事(ここではTimerThreadとしての役割であるcheck_time()の呼び出し)をすべきかどうかの判定フラグ
		// これがfalseだとidle_loop()ではシグナルが来るまで眠ったままになる。

		// ※　このタイマー監視スレッドを終了させるためには、exit = trueにしたのちに、notify_one()を呼び出す。
		// ことであるが、それはdelete_thread()がそういう処理になっている。

		if (!exit)
			sleep_condition.wait_for (mutex, run ? RESOLUTION : INT_MAX);

		mutex.unlock();

		// 思考させている状態であるなら、check_time()を呼び出して、停止すべきかチェック
		if (run)
		{
			//check_time();
		}
			
	}
}

void MainThread::IdleLoop() 
{
	while (true)
	{
		mutex.lock();

		thinking = false;

		while (!thinking && !exit)
		{
			Threads.sleep_condition.notify_one(); // Wake up UI thread if needed
			sleep_condition.wait(mutex);
		}

		mutex.unlock();

		if (exit)
			return;

		searching = true;

		//Search::think();

		assert(searching);

		searching = false;
	}
}

