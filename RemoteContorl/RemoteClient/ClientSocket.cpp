#include "pch.h"
#include "ClientSocket.h"




//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;   //��Ϊmhelper��������CSS��   ��CSS::CHELPER��������ʼ��CSS::m_helper�������

CClientSocket* pserver = CClientSocket::getInstance();   //���ܵ��ù��캯�������Ե���ָ�룬instanceȻ��newһ��CSS