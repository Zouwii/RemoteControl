#pragma once
#include "ZhrThread.h"
#include <map>
#include "CZHRQueue.h"
#include <MSWSock.h>


enum ZhrOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class ZhrServer;
class ZhrClient;
typedef std::shared_ptr<ZhrClient> PCLIENT;

class ZhrOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;   //操作 见ZhrOperator
	std::vector<char>m_buffer; //缓冲区
	ThreadWorker m_worker; //对应的处理函数
	ZhrServer* m_server; //服务器对象
};


template<ZhrOperator>class AcceptOperlapped;
typedef AcceptOperlapped<EAccept> ACCEPTOVERLAPPED;
class ZhrClient {
public:
	ZhrClient();
	~ZhrClient() {
		closesocket(m_sock);
	}

	void SetOverlapped(PCLIENT& ptr);

	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD() {
		return &m_received;
	}
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
private:
	SOCKET m_sock;
	DWORD m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};

//##################################################################
template<ZhrOperator>
class AcceptOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	AcceptOperlapped();

	int AcceptWorker();
	PCLIENT m_client;
};










template<ZhrOperator>
class RecvOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	RecvOperlapped() :m_operator(ERecv), m_worker(this, &RecvOperlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}

	int RecvWorker() {
		//todo
	}
};
typedef RecvOperlapped<ERecv> RECVOVERLAPPED;  //上面的模板



template<ZhrOperator>
class SendOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	SendOperlapped() :m_operator(ESend), m_worker(this, &SendOperlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}

	int SendWorker() {
		//todo
	}
};
typedef SendOperlapped<ESend> SENDOVERLAPPED;  //上面的模板


template<ZhrOperator>
class ErrorOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	ErrorOperlapped() :m_operator(EError), m_worker(this, &ErrorOperlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}

	int ErrorWorker() {
		//todo
	}
};
typedef ErrorOperlapped<EError> ERROROVERLAPPED;  //上面的模板



//##############################################################################

//#################################################

class ZhrServer :
	public ThreadFuncBase
{
public:
	ZhrServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
		
	}
	~ZhrServer() {}

	bool StartServer() {
		CreateSocket();
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);  //msock复用已经存在的iocp
		m_pool.Invoke();
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&ZhrServer::threadIocp));   //worker--threadIocp函数
		if (!NewAccept()) return false;
		//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&ZhrServer::threadIocp));
		//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&ZhrServer::threadIocp));
		return true;
	}	
	bool NewAccept() {
		PCLIENT pClient(new ZhrClient());  //pclient 是ZhrClient类型的智能指针
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		if (!AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient)) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}
private:
	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}


	int threadIocp() {
		DWORD transfered = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (GetQueuedCompletionStatus(m_hIOCP, &transfered, &CompletionKey, &lpOverlapped, INFINITE)) {
			if (transfered > 0 && (CompletionKey != 0)) {
				ZhrOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, ZhrOverlapped, m_overlapped);
				switch (pOverlapped->m_operator) {
				case EAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case ERecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case ESend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case EError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				}
			}
			else {
				return -1;
			}
		}
		return 0;
	}
private:

	ZhrThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<ZhrClient>> m_client; //定义就是 shared_ptr<类型>  理解为
};



