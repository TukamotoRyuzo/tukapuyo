#pragma once

#include "platform.h"
#include "field.h"
#include <vector>

struct Mutex
{
	// �N���e�B�J���Z�N�V�����I�u�W�F�N�g�̏������A�j��
	Mutex() { lock_init(l); }
	~Mutex() { lock_destroy(l); }

	// �N���e�B�J���Z�N�V�����ɓ���
	void lock() { lock_grab(l); }

	// �N���e�B�J���Z�N�V�����𔲂���
	void unlock() { lock_release(l); }

private:

	// ConditionVariable��Lock l�ɃA�N�Z�X�ł���B
	friend struct ConditionVariable;

	// �N���e�B�J���Z�N�V�����I�u�W�F�N�g
	Lock l;
};

struct ConditionVariable 
{
	// �C�x���g�I�u�W�F�N�g�̏������A�j��
	ConditionVariable() { cond_init(c); }
	~ConditionVariable() { cond_destroy(c); }

	// mutex�̃��b�N���������Ac���V�O�i����ԂɂȂ�܂ő҂B���̌ネ�b�N��������B
	void wait(Mutex& m) { cond_wait(c, m.l); }

	// mutex�̃��b�N���������Ac���V�O�i����ԂɂȂ�܂�ms�b�҂B���̌ネ�b�N��������B
	void wait_for (Mutex& m, int ms) { cond_timedwait(c, m.l, ms);}

	// �ҋ@�֐�(WaitFor)���甲����悤�ɒʒm����
	void notify_one() { cond_signal(c); }

private:
	WaitCondition c;
};

// ThreadBase�\���̂́A���ꉻ���ꂽ�X���b�h�N���X��h������N���X�K�w�̒�ӂƂȂ���̂ł���B

// ���@�h���N���X�ł́Aidle_loop()���I�[�o�[���C�h���ėp����B
// �X���b�h�̐����A��̂́Athread.cpp��new_thread()��delete_thread()��p����B

struct ThreadBase 
{
	ThreadBase() : exit(false) {}
	virtual ~ThreadBase() {}

	// �X���b�h�����������Ƃ��̊֐����Ăяo�����B
	// �h���N���X�ł́A���̊֐����I�[�o�[���C�h����B
	// ���@�^�C�}�[�X���b�h�ł���΁A���ꂪ�^�C�}�[�Ď����s�����C�������ɂ���
	// ��������A����I��check_time()���Ăяo�����B
	virtual void IdleLoop() = 0;

	// idle_loop()����sleep���Ă���̂��N����悤�ɒʒm����B
	// ���@���̃N���X�̔h���N���X��idle_loop��sleep����Ƃ��́AsleepCondition.wait_for ()��sleep������B
	// ��������΁A����notify_one()�ŋN���邱�Ƃ��ł���B
	void NotifyOne();

	// ThreadBase::wait_for ()�́A�����Ŏw�肵���ϐ�b��true�ɂȂ�܂ŃX���b�h��sleep������B
	// ���@RootPos.this_thread()->wait_for (Signals.stop);�@�̂悤�Ɏg���B
	void WaitFor(volatile const bool& b);

	Mutex mutex;
	ConditionVariable sleep_condition;
	NativeHandle handle;

	// ���̃t���O��true�ɂȂ��idle_loop()���甲����B
	// �T�����I�����Ă��邱�Ƃ������B
	// �����Œ�~���������Ƃ��́A�܂����̃t���O��true�ɂ������ƁAnotify_one()���Ăяo���B
	// ���@�܂��A�X���b�h��~�܂ő҂������Ȃ�΁AnativeThread.join()�����̂��ƂɌĂяo���K�v�����邪�A
	// thread.cpp���ɂ���delete_thread()���������������ɂȂ��Ă���B
	volatile bool exit;
};

// �^�C�}�[�Ď��X���b�h
struct TimerThread : public ThreadBase 
{
	// TimerThread�͋N������run��false�ɂ��Ă������ƂŋN�����ɂ̓^�C�}�[�Ď����s���Ă��Ȃ����Ƃ������B
	// �^�C�}�[�Ď����K�v�ɂȂ�����run��true�ɂ���ׂ��B
	TimerThread() : run(false) {}
	virtual void IdleLoop();
	bool run;

	// idle_loop()�̂Ȃ�����Acheck_time()���Ăяo�����Ԋu���~���b�P�ʂŁB
	// ���@run==true�̂Ƃ��̂�check_time()���Ăяo�����B
	// ���@check_time()�́A�v�l�̏I�����ԂɂȂ��Ă��Ȃ����𔻒肷�邽�߂̏�����������Ă���֐��B
	static const int RESOLUTION = 5; // msec between two check_time() calls
};

//�T���X���b�h
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

	// start_thinking()�́A�V�����T�����J�n���邽�߂�MainThread::idle_loop()�̂Ȃ���
	// sleep���Ă��郁�C���X���b�h���N�����A�����ɋA��B
	// �����̈Ӗ�
	// const Position&(1�ڂ̈���)�@���@�T���J�n�ǖʁB
	// Search::LimitsType(2�ڂ̈���) �� ����̎v�l���ԁA�������ԂȂǁB
	// searchMoves(3�ڂ̈���)�@���@�T������w����B�Ȃ��Ȃ�΁A��̔z���n���Ă����΁A�S���@��̈Ӗ��ƂȂ�B
	// �����Ȃ��΁A�����œn�����w����݂̂�T������B
	// ���̈����́AUCI�v���g�R����go�R�}���h�̈����Ƃ���'searchmoves'�Ƃ��Ďw��ł���B
	// Search::StateStackPtr&(4�ڂ̈���)�@���@�����̌��o�Ȃǂ����邽�߂ɒT�����̋ǖʂ�ێ����Ă����\����
//	void start_thinking(const LightField&, const Search::LimitsType&, const std::vector<Move>&);

	Mutex mutex;
	ConditionVariable sleep_condition;
	bool sleep_while_idle;
private:
	TimerThread* timer;
};

extern ThreadPool Threads;
