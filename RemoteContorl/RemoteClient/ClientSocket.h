#pragma once


#include "pch.h"
#include "framework.h"
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <mutex>

#pragma pack(push)
#pragma pack(1)


#define WM_SEND_PACK (WM_USER+1)
#define WM_SEND_PACK_ACK (WM_USER+2)//���Ͱ�����Ӧ��

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {     //����õĹ��캯��
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
				sHead = *(WORD*)(pData + i);  //һ��һ�ֽڣ���ΪBYTE
				i += 2; //��ֹ����ֻ�������ֽڵİ�
				break;
			}
		}				//�����ݿ��ܲ�ȫ�����߰�ͷû��ȫ�����յ�
		if (i + 4 + 2 + 2 > nSize) {                                   //[][] [][][][] [][] [][][][][] [][]
			nSize = 0; //û����������õ�0�ֽ�                    //head  length   cmd            sum
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;

		if (nLength + i > nSize) {            //i �ǵ�ǰ��ͷλ�ã�length�ǳ���
			nSize = 0;                        //��û����ȫ���յ�
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)                      //���� ����Ϊlength-cmd��2��-sum��2��
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			TRACE("%s\r\n", strData.c_str() + 12);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i); i += 2;  //��sum
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {                     //��У��ɹ�
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
	int Size() { //�����ݴ�С
		return nLength + 6;
	}
	const char* Data(std::string& strOut) const {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();   //ָ������ָ��ͷ
		*(WORD*)pData = sHead;// *p ��ָ  
		pData += 2;//p�ǵ�ַ
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;    //��ָ�������strOut�����

		return strOut.c_str();  //c_str: string->char

	}

public:
	WORD sHead;  //�̶�λ FE FF
	DWORD nLength; //�����ȣ��ӿ��������У�������
	WORD sCmd; //��������
	std::string strData;  //������
	WORD sSum;// ��У��
	//std::string strOut; //������������
	HANDLE hEvent;
};

//###############################################################

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����н�
	POINT ptXY;// ����


}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	char szFileName[256];
	BOOL IsDirectory; //�Ƿ�Ŀ¼ 0��1�� WENJIANJIA
	BOOL IsInvalid; // �Ƿ���Ч
	BOOL HasNext;  //0�� 1��
}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1,//clisen socket mode
};





typedef struct PacketData{
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam=0) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}

}PACKET_DATA;






//#############################################################################################
#pragma pack(pop)

std::string GetErrInfo(int wsaErrCode); //ֻ��һ������
void Dump(BYTE* pData, size_t nSize);


class CClientSocket
{
public:
	static CClientSocket* getInstance() { //������ �ѹ����������Ϊ˽��
		if (m_instance == NULL)      //��̬����û��thisָ�룬�����޷�ֱ�ӷ��ʳ�Ա����
			m_instance = new CClientSocket();
		return m_instance;
	}

	bool InitSocket();


#define BUFFER_SIZE 40960000
	int DealCommand()
	{
		if (m_sock == -1) return false;
		//char buffer[1024] = "";
		char* buffer = m_buffer.data(); //TODO:���̷߳�������ʱ���ܻ��ͻ
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0); //������պ���Ĳ��� �����ͬ��
			if (((int)len <= 0) && ((int)index==0))
			{
				return -1;
			}
			//Dump((BYTE*)buffer, index);
			//TODO: ��������
			index += len;
			len = index;
			//TRACE("!!!!!���  index:%d len:%d\n ", index, len);
			m_packet = CPacket((BYTE*)buffer, len); //��� һ��ʼindex��len���� len�᷵��һ�����Ĵ�С
			if (len > 0) {
				memmove(buffer, buffer + len, index - len); //�Ƶ�ͷ��  ÿ�η��ص�ʱ�����ջ�����
				index -= len;
				//TRACE("!!!!!������  index:%d len:%d\n ", index,len);
				return m_packet.sCmd;     //��󷵻ص���cmd
			}
		}
		
		return -1;
	}

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed=true,WPARAM wParam=0);

	bool GetFilePath(std::string& strPath) {   //����ϢӦ�þ���·��
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

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateAddress(int nIP, int nPort)
	{
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

private:
	HANDLE m_eventInvoke;
	UINT m_nThreadID;
	typedef void(CClientSocket::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC>m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	bool m_bAutoClose;
	std::list<CPacket>m_lstSend;
	std::map<HANDLE, std::list<CPacket>&>m_mapAck;
	std::map<HANDLE, bool>m_mapAutoClosed;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss);

	CClientSocket();
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	};
	static unsigned __stdcall threadEntry(void* arg);
	//void threadFunc();
	void threadFunc2();
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//TODO: ����ֵ����
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

	bool Send(const char* pData, size_t nSize)
	{
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);

	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);//��������ֵ�ͳ���





	static CClientSocket* m_instance;  //��̬=ȫ��

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


extern CClientSocket server;   //����ͷ����ͷ�������ⲿ����
//����ͷ �ⲿ��һ�����Ž�server