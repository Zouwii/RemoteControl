#pragma once

#include "pch.h"
#include "framework.h"

class CPacket
{
public:
	CPacket():sHead(0),nLength(0),sCmd(0),sSum(0){}
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
		if (i+4+2+2 >= nSize) {                                   //[][] [][][][] [][] [][][][][] [][]
			nSize = 0; //û����������õ�0�ֽ�                    //head  length   cmd            sum
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		
		if (nLength + i > nSize) {            //i �ǵ�ǰ��ͷλ�ã�length�ǳ���
			nSize = 0;                        //��û����ȫ���յ�
			return;
		}
		sCmd = *(DWORD*)(pData + i); i += 2;
		if (nLength > 4)                      //���� ����Ϊlength-cmd��2��-sum��2��
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(DWORD*)(pData + i); i += 2;  //��sum
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[i]) & 0xFF;
		}
		if (sum == sSum) {                     //��У��ɹ�
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
public:
	WORD sHead;  //�̶�λ FE FF
	DWORD nLength; //�����ȣ��ӿ��������У�������
	WORD sCmd; //��������
	std::string strData;  //������
	WORD sSum;// ��У��
};


class CServerSocket
{
public:
	static CServerSocket* getInstance() { //������ �ѹ����������Ϊ˽��
		if(m_instance==NULL)      //��̬����û��thisָ�룬�����޷�ֱ�ӷ��ʳ�Ա����
		m_instance = new CServerSocket();
		return m_instance;
	}
	
	bool InitSocket()
	{

		if (m_sock == -1)return false;
		sockaddr_in serv_adr, client_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9627);

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
		sockaddr_in client_adr;
		char buffer[1024];
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1) return false;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len=recv(m_client, buffer+index, BUFFER_SIZE -index, 0);
			if (len <= 0)
			{
				return -1;
			}
			//TODO: ��������
			index += len;
			len = index;
			m_packet=CPacket ((BYTE*)buffer, len); //return len
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len); //�Ƶ�ͷ��
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize)
	{
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
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
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
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
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;  //��̬=ȫ��

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


extern CServerSocket server;   //����ͷ����ͷ�������ⲿ����
//����ͷ �ⲿ��һ�����Ž�server