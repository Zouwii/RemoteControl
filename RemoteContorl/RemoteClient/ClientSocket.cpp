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

void Dump(BYTE* pData, size_t nSize)      //����һ�¿���
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);   //%02X  16������������Ϊ2
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}