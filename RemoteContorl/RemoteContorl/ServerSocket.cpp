#include "pch.h"
#include "ServerSocket.h"


//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
CServerSocket::CHelper CServerSocket::m_helper;   //��Ϊmhelper��������CSS��   ��CSS::CHELPER��������ʼ��CSS::m_helper�������

CServerSocket* pserver = CServerSocket::getInstance();   //���ܵ��ù��캯�������Ե���ָ�룬instanceȻ��newһ��CSS