#include "pch.h"
#include "ServerSocket.h"


//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
CServerSocket::CHelper CServerSocket::m_helper;   //因为mhelper变量来自CSS类   用CSS::CHELPER函数来初始化CSS::m_helper这个变量

CServerSocket* pserver = CServerSocket::getInstance();   //不能调用构造函数，所以调用指针，instance然后new一个CSS