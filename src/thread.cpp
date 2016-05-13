#include"thread.h"
#include<climits>


// �X���b�h�v�[���B���O�ɃX���b�h��K�v���������N�������A�X�g�b�N���Ă����B
ThreadPool Threads; // Global object

namespace 
{
	// start_routine() is the C function which is called when a new thread
	// is launched. It is a wrapper to the virtual function idle_loop().
	// �V�����X���b�h���N�����ꂽ�Ƃ��ɌĂ΂��C�̊֐�
	// idle_loop()�̃��b�p�[�֐��ł���
	extern "C" { long StartRoutine(ThreadBase* th) { th->IdleLoop(); return 0; } }


	// Helpers to launch a thread after creation and joining before delete. Must be
	// outside Thread c'tor and d'tor because object shall be fully initialized
	// when start_routine (and hence virtual idle_loop) is called and when joining.

	// �X���b�h�����̂��ƃX���b�h���N�������A�X���b�h��delete�����O��join���邽�߂̃w���p�[�Q�B
	// Thread�N���X�̃R���X�g���N�^����уf�X�g���N�^�̊O���łȂ���΂Ȃ�Ȃ��B
	// �Ȃ��Ȃ�idle_loop()���z�֐����Ăяo����Ƃ���Ajoin���Ă���Ƃ��̓I�u�W�F�N�g��
	// ���S�ɏ���������Ă��Ȃ���΂Ȃ�Ȃ��̂ŁB
	// ToDo : ���܂ЂƂӖ����킩��Ȃ��B

	// T��ThreadBase�̔h���N���X�Ɖ��肵�Ă���B
	// new T���āAstd::thread�𐶐����Aidle_loop���Ăяo���B���̂Ƃ��Ɉ����Ƃ��Ă���new�������̂�n���B
	// �܂��A���̊֐���new�������̂����^�[������B
	// T�Ƃ��ēn������̂́A
	// TimerThread : �^�C�}�[�X���b�h
	// MainThread : 1�ڂ̎v�l�X���b�h
	// Thread : 2�ڈȍ~�̎v�l�X���b�h
	// �ł���B
	template<typename T> T* NewThread()
	{
		T* th = new T();
		thread_create(th->handle, StartRoutine, th); // Will go to sleep
		return th;
	}

	// �X���b�h���~�����A���̏I����҂B
	void DeleteThread(ThreadBase* th) 
	{
		// exit�t���O��true�ɂ���B����͒T�������łɏI�����Ă��Ȃ���΂Ȃ�Ȃ��B
		th->exit = true; // Search must be already finished
		th->NotifyOne();

		// �l�C�e�B�u�X���b�h�̏I����҂B
		thread_join(th->handle); // wait for thread termination
		delete th;
	}
}//namespace

// ThreadBase::notify_one()�́A���邱�Ƃ������T���ɂ���Ƃ���idle_loop()�̂Ȃ��Ŗ����Ă���X���b�h���N�����̂Ɏg���B
void ThreadBase::NotifyOne()
{
	mutex.lock();
	sleep_condition.notify_one();
	mutex.unlock();
}

// ThreadBase::wait_for ()�́A�����Ŏw�肵���ϐ�b��true�ɂȂ�܂ŃX���b�h��sleep������B
// ���@RootPos.this_thread()->wait_for (Signals.stop);�@�̂悤�Ɏg���B
void ThreadBase::WaitFor(volatile const bool& b) {

	mutex.lock();
	while (!b) sleep_condition.wait(mutex);
	mutex.unlock();
}

// TimerThread::idle_loop() �́A�^�C�}�[�X���b�h��msec�~���b��҂��Acheck_time()���Ăяo���B
// ����msec��0�Ȃ�Athread���N�������܂Ŗ��葱����B
// �� sleepCondition�̃V�O�i���ŋN�������܂ŁB
void TimerThread::IdleLoop()
{
	while (!exit)
	{
		mutex.lock();

		// sleepCondition���V�O�i����ԂɂȂ�̂�҂B
		// �^�C���A�E�g�����́Arun��0�Ȃ�INT_MAX(������)�A�����łȂ����Resolution�����B

		// run�͎d��(�����ł�TimerThread�Ƃ��Ă̖����ł���check_time()�̌Ăяo��)�����ׂ����ǂ����̔���t���O
		// ���ꂪfalse����idle_loop()�ł̓V�O�i��������܂Ŗ������܂܂ɂȂ�B

		// ���@���̃^�C�}�[�Ď��X���b�h���I�������邽�߂ɂ́Aexit = true�ɂ����̂��ɁAnotify_one()���Ăяo���B
		// ���Ƃł��邪�A�����delete_thread()���������������ɂȂ��Ă���B

		if (!exit)
			sleep_condition.wait_for (mutex, run ? RESOLUTION : INT_MAX);

		mutex.unlock();

		// �v�l�����Ă����Ԃł���Ȃ�Acheck_time()���Ăяo���āA��~���ׂ����`�F�b�N
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

