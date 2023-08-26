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

class ZhrOverlapped {      //���� recv��send��Ҫ��
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;   //���� ��ZhrOperator
	std::vector<char>m_buffer; //������
	ThreadWorker m_worker; //��Ӧ�Ĵ�����
	ZhrServer* m_server; //����������
	PCLIENT m_client;//��Ӧ�Ŀͻ���
	WSABUF m_wsabuffer;
};


template<ZhrOperator>class AcceptOperlapped;
typedef AcceptOperlapped<EAccept> ACCEPTOVERLAPPED;

template<ZhrOperator>class RecvOperlapped;
typedef RecvOperlapped<ERecv> RECVOVERLAPPED;

template<ZhrOperator>class SendOperlapped;
typedef SendOperlapped<ESend> SENDOVERLAPPED;

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
	LPWSABUF RecvWSABuffer();
	LPWSABUF SendWSABuffer();
	DWORD& flags() { return m_flags; }

	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize() const { return m_buffer.size(); }


	int Recv() {
		int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
		if (ret <= 0) return -1;
		m_used += (size_t)ret;
		//todo :��������
		return 0;
	}
private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used;// �Ѿ�ʹ�õĻ�������С
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};


//#################################################################
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
typedef ErrorOperlapped<EError> ERROROVERLAPPED;  //�����ģ��



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

	bool StartServer();
	bool NewAccept() {
		PCLIENT pClient(new ZhrClient());  //pclient ��ZhrClient���͵�����ָ��
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


	int threadIocp();
private:

	ZhrThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<ZhrClient>> m_client; //������� shared_ptr<����>  ���Ϊ
};

