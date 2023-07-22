#pragma once


#include "pch.h"
#include "framework.h"
#include <string>

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {     //打包用的构造函数
		sHead = 0xFEFF;
		nLength = nSize + 4; //cmd+[]+sum
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);  //一跳一字节，因为BYTE
				i += 2; //防止极端只有两个字节的包
				break;
			}
		}				//包数据可能不全，或者包头没有全部接收到
		if (i + 4 + 2 + 2 >= nSize) {                                   //[][] [][][][] [][] [][][][][] [][]
			nSize = 0; //没检出包来，用掉0字节                    //head  length   cmd            sum
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;

		if (nLength + i > nSize) {            //i 是当前包头位置，length是长度
			nSize = 0;                        //包没有完全接收到
			return;
		}
		sCmd = *(DWORD*)(pData + i); i += 2;
		if (nLength > 4)                      //读包 包长为length-cmd（2）-sum（2）
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(DWORD*)(pData + i); i += 2;  //读sum
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {                     //和校验成功
			nSize = i; //head2 length4 data
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack)
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() { //包数据大小
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();   //指针现在指到头
		*(WORD*)pData = sHead;// *p 是指  
		pData += 2;//p是地址
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;    //用指针完成了strOut的填充

		return strOut.c_str();  //c_str: string->char

	}

public:
	WORD sHead;  //固定位 FE FF
	DWORD nLength; //包长度（从控制命令到和校验结束）
	WORD sCmd; //控制命令
	std::string strData;  //包数据
	WORD sSum;// 和校验
	std::string strOut; //整个包的数据
};


typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、中建
	POINT ptXY;// 坐标


}MOUSEEV, * PMOUSEEV;


//#############################################################################################
#pragma pack(pop)

std::string GetErrInfo(int wsaErrCode); //只留一个定义


class CClientSocket
{
public:
	static CClientSocket* getInstance() { //单例： 把构造和析构设为私有
		if (m_instance == NULL)      //静态函数没有this指针，所以无法直接访问成员变量
			m_instance = new CClientSocket();
		return m_instance;
	}

	bool InitSocket(const std::string& strIPAddress)
	{

		if (m_sock == -1)return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = inet_addr(strIPAddress.c_str());
		serv_adr.sin_port = htons(9627);


		if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的IP地址不存在！");
			return(false);
		}

		int ret=connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1) {
			AfxMessageBox("连接失败！");
			TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return(false);
		}
		return true;
	}


#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_sock == -1) return false;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				return -1;
			}
			//TODO: 处理命令
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //return len
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len); //移到头部
				index -= len;
				return m_packet.sCmd;     //最后返回的是cmd
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize)
	{
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack)
	{
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath) {   //包信息应该就是路径
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}


private:
	
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}

	CClientSocket() {
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);

	};
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//TODO: 返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CClientSocket* m_instance;  //静态=全局

	class CHelper {
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};


extern CClientSocket server;   //引用头，在头中声明外部变量
//告诉头 外部有一个符号叫server