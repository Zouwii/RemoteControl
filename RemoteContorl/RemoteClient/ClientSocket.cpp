#include "pch.h"
#include "ClientSocket.h"




//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;   //因为mhelper变量来自CSS类   用CSS::CHELPER函数来初始化CSS::m_helper这个变量

CClientSocket* pserver = CClientSocket::getInstance();   //不能调用构造函数，所以调用指针，instance然后new一个CSS