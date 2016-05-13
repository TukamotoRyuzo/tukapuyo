#pragma once

#include "platform.h"
#include "field.h"
#include <vector>

struct Mutex
{
	// クリティカルセクションオブジェクトの初期化、破棄
	Mutex() { lock_init(l); }
	~Mutex() { lock_destroy(l); }

	// クリティカルセクションに入る
	void lock() { lock_grab(l); }

	// クリティカルセクションを抜ける
	void unlock() { lock_release(l); }

private:

	// ConditionVariableはLock lにアクセスできる。
	friend struct ConditionVariable;

	// クリティカルセクションオブジェクト
	Lock l;
};

struct ConditionVariable 
{
	// イベントオブジェクトの初期化、破棄
	ConditionVariable() { cond_init(c); }
	~ConditionVariable() { cond_destroy(c); }

	// mutexのロックを解除し、cがシグナル状態になるまで待つ。その後ロックをかける。
	void wait(Mutex& m) { cond_wait(c, m.l); }

	// mutexのロックを解除し、cがシグナル状態になるまでms秒待つ。その後ロックをかける。
	void wait_for (Mutex& m, int ms) { cond_timedwait(c, m.l, ms);}

	// 待機関数(WaitFor)から抜けるように通知する
	void notify_one() { cond_signal(c); }

private:
	WaitCondition c;
};

// ThreadBase構造体は、特殊化されたスレッドクラスを派生するクラス階層の底辺となるものである。

// ※　派生クラスでは、idle_loop()をオーバーライドして用いる。
// スレッドの生成、解体は、thread.cppのnew_thread()とdelete_thread()を用いる。

struct ThreadBase 
{
	ThreadBase() : exit(false) {}
	virtual ~ThreadBase() {}

	// スレッドが生成されるとこの関数が呼び出される。
	// 派生クラスでは、この関数をオーバーライドする。
	// ※　タイマースレッドであれば、これがタイマー監視を行うメイン処理にして
	// ここから、定期的にcheck_time()が呼び出される。
	virtual void IdleLoop() = 0;

	// idle_loop()内でsleepしているのを起きるように通知する。
	// ※　このクラスの派生クラスのidle_loopでsleepするときは、sleepCondition.wait_for ()でsleepさせる。
	// そうすれば、このnotify_one()で起きることができる。
	void NotifyOne();

	// ThreadBase::wait_for ()は、引数で指定した変数bがtrueになるまでスレッドをsleepさせる。
	// ※　RootPos.this_thread()->wait_for (Signals.stop);　のように使う。
	void WaitFor(volatile const bool& b);

	Mutex mutex;
	ConditionVariable sleep_condition;
	NativeHandle handle;

	// このフラグがtrueになればidle_loop()から抜ける。
	// 探索が終了していることが条件。
	// そこで停止させたいときは、まずこのフラグをtrueにしたあと、notify_one()を呼び出す。
	// ※　また、スレッド停止まで待ちたいならば、nativeThread.join()もそのあとに呼び出す必要があるが、
	// thread.cpp内にあるdelete_thread()がそういう実装になっている。
	volatile bool exit;
};

// タイマー監視スレッド
struct TimerThread : public ThreadBase 
{
	// TimerThreadは起動時にrunをfalseにしておくことで起動時にはタイマー監視が行われていないことを示す。
	// タイマー監視が必要になったらrunをtrueにするべき。
	TimerThread() : run(false) {}
	virtual void IdleLoop();
	bool run;

	// idle_loop()のなかから、check_time()が呼び出される間隔をミリ秒単位で。
	// ※　run==trueのときのみcheck_time()が呼び出される。
	// ※　check_time()は、思考の終了時間になっていないかを判定するための処理が書かれている関数。
	static const int RESOLUTION = 5; // msec between two check_time() calls
};

//探索スレッド
struct Thread : public ThreadBase {

	Thread();
	virtual void IdleLoop();
	bool AvailableTo(const Thread* master) const;

	volatile bool searching;
};

struct MainThread : public Thread 
{
	MainThread() : thinking(true) {} // Avoid a race with start_thinking()
	virtual void IdleLoop();
	volatile bool thinking;
};

/// ThreadPool struct handles all the threads related stuff like init, starting,
/// parking and, the most important, launching a slave thread at a split point.
/// All the access to shared thread data is done through this class.

struct ThreadPool : public std::vector<Thread*> {

public:
	void init(); // No c'tor and d'tor, threads rely on globals that should
	void exit(); // be initialized and valid during the whole thread lifetime.

	MainThread* Main() { return static_cast<MainThread*>((*this)[0]); }
	TimerThread* Timer() { return timer; }
	void WakeUp();
	void Sleep();
	void SetTimer();

	Thread* AvailableSlave(const Thread* master) const;
	void WaitForThinkFinished();
	void StartThinking();

	// start_thinking()は、新しい探索を開始するためにMainThread::idle_loop()のなかで
	// sleepしているメインスレッドを起こし、即座に帰る。
	// 引数の意味
	// const Position&(1つ目の引数)　→　探索開始局面。
	// Search::LimitsType(2つ目の引数) → 今回の思考時間、持ち時間など。
	// searchMoves(3つ目の引数)　→　探索する指し手。ないならば、空の配列を渡しておけば、全合法手の意味となる。
	// さもなくば、ここで渡した指し手のみを探索する。
	// この引数は、UCIプロトコルのgoコマンドの引数として'searchmoves'として指定できる。
	// Search::StateStackPtr&(4つ目の引数)　→　千日手の検出などをするために探索中の局面を保持しておく構造体
//	void start_thinking(const LightField&, const Search::LimitsType&, const std::vector<Move>&);

	Mutex mutex;
	ConditionVariable sleep_condition;
	bool sleep_while_idle;
private:
	TimerThread* timer;
};

extern ThreadPool Threads;
