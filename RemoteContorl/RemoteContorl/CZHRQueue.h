#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "ZhrThread.h"


template <class T>
class CZHRQueue
{//线程安全队列 利用IOCP实现
public:
	enum {
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear,
	};

	typedef struct IocpParam {
		size_t nOperator;//操作
		T Data;//数据
		HANDLE hEvent; //pop需要
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM; //PostParameter 用于投递信息结构体

	CZHRQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&CZHRQueue::threadEntry, 0, this);
		}
	}
	virtual ~CZHRQueue() {
		if (m_lock)return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hCompletionPort, INFINITE);
		if (m_hCompletionPort != NULL) {
			HANDLE htemp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(htemp);
		}
	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("push back done!  %d  %08p \r\n", ret,(void*)pParam);
		return ret;
	}
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQPop, data, hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		if (m_lock) return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		return ret;
	}
protected:
	static void threadEntry(void* arg) {
		CZHRQueue<T>* thiz = (CZHRQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}

	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case EQPush:
		{
			m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n",(void*)pParam);
		}
		break;
		case EQPop:
		{
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
		}
		break;
		case EQSize:
		{
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
		}
		break;
		case EQClear:
		{
			m_lstData.clear();
			delete pParam;
		}
		break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}

	virtual void threadMain() {
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, INFINITE)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepared to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			//printf("%08X \r\n", pParam);
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, 0)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepared to exit   ========  !\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
		//m_hCompletionPort = NULL;
	}
protected:
	std::list <T>m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool>m_lock;//队列正在析构
};

//#####################################################

template<class T>
class ZhrSendQueue :public CZHRQueue<T>,public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* ZHRCALLBACK)(T& data);
	ZhrSendQueue(ThreadFuncBase* obj, ZHRCALLBACK callback)
		:CZHRQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdataWorker(::ThreadWorker(this, (FUNCTYPE)&ZhrSendQueue<T>::threadTick));
	}
	virtual ~ZhrSendQueue(){
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}

protected:
	virtual bool PopFront(T& data) {
		return false;
	}
	bool PopFront()
	{
		typename CZHRQueue<T>::IocpParam* Param=new typename CZHRQueue<T>::IocpParam(CZHRQueue<T>::EQPop, T());
		if (CZHRQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CZHRQueue<T>::m_hCompletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() {
		if (WaitForSingleObject(CZHRQueue<T>::m_hThread, 0) != WAIT_TIMEOUT) return 0;
		if (CZHRQueue<T>::m_lstData.size() > 0 ) {
			PopFront();
		}
		return 0;
	}

	virtual void DealParam(typename CZHRQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator) {
		case CZHRQueue<T>::EQPush:
		{
			CZHRQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n",(void*)pParam);
		}
		break;
		case CZHRQueue<T>::EQPop:
		{
			if (CZHRQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CZHRQueue<T>::m_lstData.front();
				if((m_base->*m_callback)(pParam->Data)==0)
					CZHRQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
		}
		break;
		case CZHRQueue<T>::EQSize:
		{
			pParam->nOperator = CZHRQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
		}
		break;
		case CZHRQueue<T>::EQClear:
		{
			CZHRQueue<T>::m_lstData.clear();
			delete pParam;
		}
		break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}

private:
	ThreadFuncBase* m_base;
	ZHRCALLBACK m_callback;
	ZhrThread m_thread;
};

typedef ZhrSendQueue<std::vector<char>>::ZHRCALLBACK SENDCALLBACK;