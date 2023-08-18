#include "pch.h"
#include "ClientSocket.h"




//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;   //因为mhelper变量来自CSS类   用CSS::CHELPER函数来初始化CSS::m_helper这个变量

CClientSocket* pserver = CClientSocket::getInstance();   //不能调用构造函数，所以调用指针，instance然后new一个CSS

//冲突，所以在cpp中实现
std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;

}

void Dump(BYTE* pData, size_t nSize)      //导出一下看看
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);   //%02X  16进制输出，宽度为2
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}


CClientSocket::CClientSocket() :
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, "");
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if(WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT){
		TRACE("网络消息处理线程启动失败\r\n");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK,&CClientSocket::SendPack},//绑定
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>
			(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入失败，消息\r\n");
		}
	}
}




bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)return false;
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(m_nIP);
	serv_adr.sin_port = htons(m_nPort); //9527


	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox("指定的IP地址不存在！");
		return(false);
	}

	int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
	if (ret == -1) {
		AfxMessageBox("连接失败！");
		TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return(false);
	}
	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret=PostThreadMessage(m_nThreadID, WM_SEND_PACK,(WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;
	}
	return ret;
}



//bool CClientSocket::SendPacket(const CPacket& pack,
//	std::list<CPacket>& lstPacks, bool isAutoClosed)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
//		/*	if (InitSocket() == false) return false;*/
//		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
//	}
//	//PostThreadMessage()
//	m_lock.lock();
//	auto pr = m_mapAck.insert(std::pair<HANDLE,
//		std::list<CPacket>&>(pack.hEvent, lstPacks));
//
//	m_mapAutoClosed.insert(std::pair<HANDLE, bool>
//		(pack.hEvent, isAutoClosed));
//
//	m_lstSend.push_back(pack);
//	m_lock.unlock();
//	WaitForSingleObject(pack.hEvent, INFINITE);
//	std::map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = m_mapAck.find(pack.hEvent);
//	if (it != m_mapAck.end()) {
//		std::list<CPacket>::iterator i;
//		m_lock.lock();
//		m_mapAck.erase(it);
//		m_lock.unlock();
//		return true;
//	}
//	return false;
//}

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message:%08X \r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);//相当于MSGFUNC(参数)
		}
	}
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

//void CClientSocket::threadFunc()
//{
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//	int index = 0;
//	InitSocket();
//	while (m_sock != INVALID_SOCKET) {
//		if (m_lstSend.size() > 0) {
//			TRACE("lstSend: size %d\r\n", m_lstSend.size());
//			m_lock.lock();
//			CPacket& head = m_lstSend.front();
//			m_lock.unlock();
//			if (Send(head) == false) {
//				TRACE("发送失败！\r\n");
//				continue;
//			}
//			std::map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = m_mapAck.find(head.hEvent);
//			if (it != m_mapAck.end()) {
//				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
//				do {
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
//					if (length > 0 || index > 0) {
//						index += length;
//						size_t size = (size_t)index;
//						CPacket pack((BYTE*)pBuffer, size); //和校验成功，size才会大于0
//						if (size > 0) {
//							//TODO:通知对应的时间
//							pack.hEvent = head.hEvent;
//							it->second.push_back(pack);
//							//SetEvent(head.hEvent);
//							memmove(pBuffer, pBuffer + size, index - size);//recv之后要移动包
//							index -= size;
//							if (it0->second) {
//								SetEvent(head.hEvent);
//								break;
//							}
//						}
//					}
//					else if (length <= 0 && index <= 0) {
//						CloseSocket();
//						SetEvent(head.hEvent);//等到服务器关闭命令，再通知事情完成
//						if (it0 != m_mapAutoClosed.end())
//						{
//							TRACE("Set Event %d %d\r\n", head.sCmd, it0->second);
//						}
//						else {
//							TRACE("异常情况,没有对应pair\r\n");
//						}
//						break;
//					}
//				} while (it0->second == false);
//			}
//			m_lock.lock();
//			m_lstSend.pop_front(); //删除头部
//			m_mapAutoClosed.erase(head.hEvent);
//			m_lock.unlock();
//			if (InitSocket() == false) {
//				InitSocket();
//			}
//		}
//		Sleep(1);
//	}
//	CloseSocket();
//}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock=%d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	//pack.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//定义消息数据结构（数据和长度、模式） 回调消息的数据结构HWND
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	if (InitSocket() == true)
	{
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock,pBuffer+index ,BUFFER_SIZE-index, 0);
				if ((length > 0)||(index>0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					//收到就会用消息发回来
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE)
						{
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
				}
				else {//TODO:对方关闭套接字或者网络异常
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else {
			CloseSocket();//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {//错误处理
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL,-2);
	}
}
