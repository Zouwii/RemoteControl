#include "pch.h"
#include "ClientSocket.h"




//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;   //��Ϊmhelper��������CSS��   ��CSS::CHELPER��������ʼ��CSS::m_helper�������

CClientSocket* pserver = CClientSocket::getInstance();   //���ܵ��ù��캯�������Ե���ָ�룬instanceȻ��newһ��CSS

//��ͻ��������cpp��ʵ��
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