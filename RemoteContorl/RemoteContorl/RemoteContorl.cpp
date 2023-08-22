// RemoteContorl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteContorl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "ZHRTool.h"
//#pragma commit(linker, "/subsystem:windows /entry:mainCRTStartup")

// 唯一的应用程序对象

#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteContorl.exe")


CWinApp theApp;
using namespace std;



bool ChooseAutoInvoke(const CString& strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");
	//GetSystemDirectory(wcsSystem, MAX_PATH);
	if (PathFileExists(strPath)) {
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法用途！\n");
	strInfo += _T("继续运行，将使得这台机器处于被监控状态\n");
	strInfo += _T("停止请按“取消”，退出程序\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到机器上，并且随系统自动运行\n");
	strInfo += _T("按下“否”按钮，该程序只会运行一次\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		CZHRTool::WriteRegisterTable(strPath);
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}

//###########################################################################################


typedef struct IocpParam {
	int nOperator;//操作
	std::string strData;//数据
	_beginthread_proc_type cbFunc;//回调函数
	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
		nOperator = op;
		strData = sData;
		cbFunc = cb;  //func(pstr)
	}
	IocpParam() {
		nOperator = -1;
	}
}IOCP_PARAM;

enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};

void threadmain(HANDLE hIOCP) {
	std::list<std::string> lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;
	int count = 0, count0 = 0;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
		if ((dwTransferred == 0) || (CompletionKey == NULL)) {
			printf("thread is prepared to exit!\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
			count0++;
		}
		else if (pParam->nOperator == IocpListPop) {
			std::string* pStr = NULL;
			if (lstString.size() > 0) {
				pStr = new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(pStr); //"hello world"
			}
			count++;
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}
		delete pParam;
	}
	//lstString.clear();
	printf("thread exit! count: %d  count0: %d \r\n", count, count0);
}

void threadQueryEntry(HANDLE hIOCP) { //工作线程
	threadmain(hIOCP);
	_endthread(); //
}


void func(void* arg) {
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("pop from list:%s\r\n", pstr->c_str());
		delete pstr;
	}
	else {
		printf("list is empty,no data\r\n");
	}
}


void fun1()
{

}


int main()
{
	if (!CZHRTool::Init())return 1;

	printf("press any key to exit...\r\n");
	HANDLE  hIOCP = INVALID_HANDLE_VALUE;
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//epoll 区别 epoll是一个线程处理
	if ((hIOCP == INVALID_HANDLE_VALUE) || (hIOCP == NULL)) {
		printf("create iocp failed!%d\r\n", GetLastError());
	}
	HANDLE hThread = (HANDLE)_beginthread(threadQueryEntry, 0, hIOCP);


	ULONGLONG tick = GetTickCount64();
	ULONGLONG tick0 = GetTickCount64();
	int count = 0, count0 = 0;

	while (_kbhit() == 0) {   //完成端口 把请求和实现分离
		if (GetTickCount64() - tick0 > 1300) {
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world", func), NULL);
			tick0 = GetTickCount64();
			count++;
		}
		if (GetTickCount64() - tick > 2000) {
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL);
			tick = GetTickCount64();
			count0++;
		}
		Sleep(1);
	}
	if (hIOCP != NULL) {
		//TODO:唤醒完成端口  结束要唤醒，保证线程over
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
		WaitForSingleObject(hIOCP, INFINITE);
	}
	CloseHandle(hIOCP);
	printf("exit done! count: %d  count0:%d  \r\n", count, count0);
	::exit(0);





	//####################################################
   /* if (CZHRTool::IsAdamin()) {
		if (!CZHRTool::Init())return 1;
		if (ChooseAutoInvoke(INVOKE_PATH)) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(NULL, _T("网络初始化异常，请检查网络连接！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else {
		if (CZHRTool::RunAsAdmin() == false){
			CZHRTool::ShowError();
			return 1;
		}
	}
	return 0;*/
}
