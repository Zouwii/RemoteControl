#pragma once
#include <atomic>
#include <vector>
#include <mutex>
#include "pch.h"
#include <Windows.h>

class ThreadFuncBase{};
typedef int (ThreadFuncBase::* FUNCTYPE)();  //FUNCTYPE就是新定义的pfunc函数指针  继承然后就可以调用了

class ThreadWorker {
public:
	ThreadWorker() :thiz(NULL), func(NULL) {}

	ThreadWorker(void* obj, FUNCTYPE f):thiz((ThreadFuncBase*)obj),func(f) {}

	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}

	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {
		if (thiz) {
			return(thiz->*func)();
		}
		return -1;
	}
	bool IsValid()const {
		return((thiz != NULL) && (func != NULL));
	}
private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};


class ZhrThread
{
public:
	ZhrThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~ZhrThread()
	{
		Stop();
	}
	bool Start() { //true表示成功 false失败
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&ZhrThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() { //true表示有效 false表示线程异常或者终止
		if ((m_hThread == NULL) || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
	bool Stop()
	{
		if (m_bStatus == false) return true;
		m_bStatus = false;
		DWORD ret=WaitForSingleObject(m_hThread, 1000);
		UpdataWorker();
		if (ret == WAIT_TIMEOUT) {
			TerminateThread(m_hThread, -1);
		}
		UpdataWorker();
		return ret=WAIT_OBJECT_0;
	}
	void UpdataWorker(const::ThreadWorker& worker=::ThreadWorker()) {
		if (m_worker.load() != NULL&&(m_worker.load()!=&worker)) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker)return;
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		
		m_worker.store(new::ThreadWorker(worker));
	}

	bool IsIdle() {//ture 空闲  false 已经分配了工作
		if (m_worker.load() == NULL)return true;
	    return	!m_worker.load()->IsValid();
	}

private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker= *m_worker.load();
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();
					if (ret != 0) {
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0) {
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL);
						delete pWorker;
					}
				}
			}
			else {
				Sleep(1);
			}

		}
	}

	static void ThreadEntry(void* arg) {
		ZhrThread* thiz = (ZhrThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	bool m_bStatus; //false表示将要关闭 true表示正在运行
	HANDLE m_hThread;
	std::atomic<::ThreadWorker*> m_worker;
};


class ZhrThreadPool
{
public:
	ZhrThreadPool(size_t size){
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++)
			m_threads[i] = new ZhrThread();
	}
	ZhrThreadPool() {}
	~ZhrThreadPool(){
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++) {
			delete m_threads[i];
			m_threads[i] = NULL;
		}
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	//返回-1表示分配失败，所有线程都在忙  大于等于0，表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->IsIdle()) {   //这个效率差一点 可以优化
				m_threads[i]->UpdataWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}

private:
	std::mutex m_lock;
	std::vector<ZhrThread*> m_threads;
};