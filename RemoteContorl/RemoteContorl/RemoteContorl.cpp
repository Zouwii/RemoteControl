// RemoteContorl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteContorl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma commit(linker, "/subsystem:windows /entry:mainCRTStartup")

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize)      //导出一下看看
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut +="\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);   //%02X  16进制输出，宽度为2
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

std::string MakeDriverInfo() { //1->A 2->B 3->C  26->Z
    std::string result;
    for (int i = 1; i <= 26; i++)
    {
        if (_chdrive(i) == 0) {
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data(), pack.Size());
    //CServerSocket::getInstance()->Send(pack);
    return 0;
}

typedef struct file_info{
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
}FILEINFO,*PFILEINFO;

int MakeDirectoryInfo() {                  //指定目录下的文件和文件夹
    std::string strPath;
    std::list<FILEINFO> lstFileInfos;   //<>泛型 比如list<string>

    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {     //get 把 strpath设置为了包里面的信息
        OutputDebugString(_T("当前命令不是获取文件列表，命令解析失败！"));
        return -1;
    }
    if (_chdir(strPath.c_str())!=0) {   //失败的情况
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());   //这里把文件名拷到finfo来
        //lstFileInfos.push_back(finfo);
        CPacket pack(2,(BYTE*)&finfo,sizeof(finfo));    //这样看是把finfo发出去了
        CServerSocket::getInstance()->Send(pack);   //TODO:返回值
        OutputDebugString(_T("没有权限访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind=_findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件！"));
        return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
        //finfo.IsInvalid = FALSE;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name)); //把找到文件名放进list中
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));    //这样看是把finfo发出去了
        CServerSocket::getInstance()->Send(pack);   //TODO:返回值

        //lstFileInfos.push_back(finfo);
    } while (!_findnext(hfind, &fdata));
    //发送信息到控制端  但是文件夹文件太多会有问题
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));    //这样看是把finfo发出去了
    CServerSocket::getInstance()->Send(pack);   //TODO:返回值

    return 0;
}

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
            //{
               //CServerSocket local;
            //}
            //CServerSocket* pserver= CServerSocket::getInstance();   //创建单例
            //int count = 0;
            //if (pserver->InitSocket() == false)
            //{
            //    MessageBox(NULL, _T("网络初始化异常，请检查网络连接！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while(CServerSocket::getInstance()!=NULL)
            //{
            //    if (pserver->AcceptClient() == false)
            //    {
            //        if (count >= 3)
            //        {
            //            MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    if (pserver->DealCommand());
                //TODO:
            //}
            int nCmd = 1;
            switch (nCmd)
            {
            case 1: //查看磁盘分区
                MakeDriverInfo();
                break;
            case 2: //查看指定目录下的文件
                MakeDirectoryInfo();

                break;
            }
            //TODO:
           

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
