#include "pch.h"
#include "ZhrServer.h"

template<ZhrOperator op>
AcceptOperlapped<op>::AcceptOperlapped()
{
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOperlapped<op>::AcceptWorker);
	m_operator = EAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template<ZhrOperator op>
int AcceptOperlapped<op>::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (*(LPDWORD)m_client.get() > 0) {
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength, //本地地址
			(sockaddr**)m_client->GetRemoteAddr(), &rLength //远程地址
		);
		//RECVOVERLAPPED
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), *m_client, NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:报错
		}

		if (!m_server->NewAccept()) { //accpet后开启下一轮accept
			return -2;
		}
	}
	return -1;
}

template <ZhrOperator op>
inline SendOperlapped<op>::SendOperlapped()
{
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOperlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}
template <ZhrOperator op>
inline RecvOperlapped<op>::RecvOperlapped()
{
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOperlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}


ZhrClient::ZhrClient()
	:m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED),
	m_send(new SENDOVERLAPPED)
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void ZhrClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
}

ZhrClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

LPWSABUF ZhrClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPWSABUF ZhrClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

bool ZhrServer::StartServer()
{
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

int ZhrServer::threadIocp()
{

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
