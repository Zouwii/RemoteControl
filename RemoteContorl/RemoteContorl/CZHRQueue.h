#pragma once

template <class T>

class CZHRQueue
{//�̰߳�ȫ���� ����IOCPʵ��
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
		int nOperator;//����
		T strData;//����
		_beginthread_proc_type cbFunc;//�ص�����
		HANDLE hEvent; //pop��Ҫ
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM; //PostParameter ����Ͷ����Ϣ�ṹ��

	enum {
		EQPush,
		EQPop,
		EQSize,
		EQClear,
	};
};


