#pragma once

#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)

void Dump(BYTE* pData, size_t nSize);

class CPacket
{
public:
	CPacket():sHead(0),nLength(0),sCmd(0),sSum(0){}
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
	CPacket(const BYTE* pData, size_t& nSize) {   //解包
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);  //一跳一字节，因为BYTE
				i += 2; //防止极端只有两个字节的包
				break;
			}
		}				//包数据可能不全，或者包头没有全部接收到
		if (i+4+2+2 > nSize) {                                   //[][] [][][][] [][] [][][][][] [][]    //!!!!!!!!!!!!!!!
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
	~CPacket(){}
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
		*(WORD*) pData = sHead;// *p 是指  
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


typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、中建
	POINT ptXY;// 坐标


}MOUSEEV,*PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	char szFileName[256];
	BOOL IsDirectory; //是否目录 0否1是 WENJIANJIA
	BOOL IsInvalid; // 是否有效
	BOOL HasNext;  //0无 1有
}FILEINFO, * PFILEINFO;


//#############################################################################################

//SERVER SOCKET!!!!

//###########################################################################################
#pragma pack(pop)
class CServerSocket
{
public:
	static CServerSocket* getInstance() { //单例： 把构造和析构设为私有
		if (m_instance == NULL)      //静态函数没有this指针，所以无法直接访问成员变量
			m_instance = new CServerSocket();
		return m_instance;
	}

	bool InitSocket()
	{

		if (m_sock == -1)return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);

		//bind
		int ret = bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1)
		{
			return false;
		}
		if (listen(m_sock, 1) == -1)
		{
			return false;
		}
		return true;
	}
	bool AcceptClient()
	{
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_adr;
		char buffer[1024];
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client=%d\r\n",m_client);
		if (m_client == -1) return false;
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand()              //把recv的包放进m_packet中
	{
		if (m_client == -1) return false;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL)
		{
			TRACE("服务端内存不足\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				delete[] buffer;
				return -1;
			}
			TRACE("DealCommand-- recv:%d\r\n", len);
			//TODO: 处理命令
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); // 此时把buffer解读为m_packet 执行完后len=i
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len); //移到头部
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;     //异常的情况，最后返回的是cmd
			}
			
		}
		delete[] buffer;
		return -1;
	}

	bool Send(const char* pData, size_t nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack)
	{
		if (m_client == -1) return false;
		Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
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

	CPacket& GetPacket() {
		return m_packet;
	}
	void CloseClient() {
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}

private:
	SOCKET m_client;
	SOCKET m_sock;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}

	CServerSocket() {
		m_client = INVALID_SOCKET;

		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}

		m_sock = socket(PF_INET, SOCK_STREAM, 0);

	};
	~CServerSocket() {
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
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;  //静态=全局

	class CHelper {
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};


extern CServerSocket server;   //引用头，在头中声明外部变量
//告诉头 外部有一个符号叫server