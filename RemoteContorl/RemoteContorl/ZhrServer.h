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

class ZhrOverlapped {      //父类 recv和send都要用
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;   //操作 见ZhrOperator
	std::vector<char>m_buffer; //缓冲区
	ThreadWorker m_worker; //对应的处理函数
	ZhrServer* m_server; //服务器对象
	ZhrClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;
	virtual ~ZhrOverlapped() {
		m_buffer.clear();
	}
};


template<ZhrOperator>class AcceptOperlapped;
typedef AcceptOperlapped<EAccept> ACCEPTOVERLAPPED;

template<ZhrOperator>class RecvOperlapped;
typedef RecvOperlapped<ERecv> RECVOVERLAPPED;

template<ZhrOperator>class SendOperlapped;
typedef SendOperlapped<ESend> SENDOVERLAPPED;


//#####################################################
//CLIENT
class ZhrClient: public ThreadFuncBase{
public:
	ZhrClient();
	~ZhrClient() {
		m_buffer.clear();
		closesocket(m_sock);
		m_recv.reset();
		m_send.reset();
		m_overlapped.reset();
		m_vecSend.Clear();
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
	LPWSABUF RecvWSABuffer();
	LPWSAOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPWSAOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }

	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize() const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);

private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used;// 已经使用的缓冲区大小
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	ZhrSendQueue<std::vector<char>> m_vecSend;//发送数据队列
};


//#################################################################
//template
template<ZhrOperator>
class AcceptOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	AcceptOperlapped();
	int AcceptWorker();
};


template<ZhrOperator>
class RecvOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	RecvOperlapped();

	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
};



template<ZhrOperator>
class SendOperlapped :public ZhrOverlapped, ThreadFuncBase
{
public:
	SendOperlapped();

	int SendWorker() {
		//todo
		return -1;
	}
};



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


//##############################################################
//SERVER

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
	~ZhrServer();

	bool StartServer();
	bool NewAccept();
	void BindNewSocket(SOCKET s);
private:
	void CreateSocket();
	int threadIocp();
private:

	ZhrThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<ZhrClient>> m_client; //定义就是 shared_ptr<类型>  理解为
};

