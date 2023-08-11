﻿// RemoteContorl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteContorl.h"
#include "ServerSocket.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "ZHRTool.h"
//#pragma commit(linker, "/subsystem:windows /entry:mainCRTStartup")

// 唯一的应用程序对象

CWinApp theApp;
using namespace std;

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CCommand cmd;
            CServerSocket* pserver= CServerSocket::getInstance();   //创建单例
            int count = 0;
            if (pserver->InitSocket() == false)
            {
                MessageBox(NULL, _T("网络初始化异常，请检查网络连接！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while(CServerSocket::getInstance()!=NULL)
            {
                if (pserver->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                    count++;
                }
                TRACE("AcceptClient return true\r\n");
                int ret=pserver->DealCommand(); //dealcommand返回sCmd
                TRACE("DealCommand ret=%d\r\n", ret);
                if (ret > 0)                
                {
                    ret=cmd.ExecuteCommand(pserver->GetPacket().sCmd);   //##目标 走进execute 执行1981-》TestConnect

                    if (ret != 0)
                    {
                        TRACE("执行命令失败：%d ret=%d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("command has done!\r\r");
                }
                
                //TODO:
            }
            //ExecuteCommand(1);
        }
    }
    else
    { 
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
