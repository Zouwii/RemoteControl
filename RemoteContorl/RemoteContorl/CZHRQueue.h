#pragma once

template <class T>

class CZHRQueue
{//线程安全队列 利用IOCP实现
public:
	CZHRQueue();
	~CZHRQueue();
	bool Push(const T& data);
	bool Pop(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::list <T>m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_Thread;
public:
	typedef struct IocpParam {
		int nOperator;//操作
		T strData;//数据
		_beginthread_proc_type cbFunc;//回调函数
		HANDLE hEvent; //pop需要
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM; //PostParameter 用于投递信息结构体

	enum {
		EQPush,
		EQPop,
		EQSize,
		EQClear,
	};
};


