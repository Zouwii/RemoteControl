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