// RemoteContorl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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


void WriteRegisterTable(const CString& strPath)
{
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    char sPath[MAX_PATH] = "";
    char sSys[MAX_PATH] = "";
    std::string strExe = "\\RemoteContorl.exe ";
    GetCurrentDirectoryA(MAX_PATH, sPath);
    GetSystemDirectoryA(sSys, sizeof(sSys));
    std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
    system(strCmd.c_str());
    HKEY hKey = NULL;
    int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey);
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n 程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n 程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    RegCloseKey(hKey);
}




void ChooseAutoInvoke() {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    //GetSystemDirectory(wcsSystem, MAX_PATH);
    CString strPath =CString(_T("C:\\Windows\\SysWOW64\\RemoteContorl.exe"));
    if(PathFileExists(strPath)){
        return;
    }
    CString strInfo = _T("该程序只允许用于合法用途！\n");
    strInfo += _T("继续运行，将使得这台机器处于被监控状态\n");
    strInfo += _T("停止请按“取消”，退出程序\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到机器上，并且随系统自动运行\n");
    strInfo += _T("按下“否”按钮，该程序只会运行一次\n");
    int ret=MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        WriteRegisterTable(strPath);
    }
    else if (ret == IDCANCEL) {
        exit(0);
    }
    return;
}

void ShowError() {
    LPWSTR lpMessageBuf = NULL;
    //strerror(errno);//标准c语言库
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&lpMessageBuf, 0, NULL);   //GETLASTERROR只显示当前线程错误
    OutputDebugString(lpMessageBuf);
    LocalFree(lpMessageBuf);
}

bool IsAdamin() {
    HANDLE hToken = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        ShowError();
        return false;
    }
    TOKEN_ELEVATION eve;
    DWORD len = 0;
    if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
        ShowError();
        return false;
    }
    CloseHandle(hToken);
    if (len == sizeof(eve)) {
        return eve.TokenIsElevated;
    }
    //不等于说明其他权限
    printf("length of tokeninformation is %d\r\n", len);
    return false;

}

void RunAsAdmin()
{
    HANDLE hToken = NULL;
    BOOL ret=LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
    if (!ret) {
        ShowError();
        MessageBox(NULL, _T("登录错误！"), _T("程序错误"), 0);
        ::exit(0);
    }
    OutputDebugString(L"logon Administrator success!\r\n");
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    TCHAR sPath[MAX_PATH] = _T("");
    GetCurrentDirectory(MAX_PATH, sPath); //当前进程路径
    CString strCmd = sPath;
    strCmd += _T("\\RemoteContorl.exe"); //创建进程
    //ret=CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    CloseHandle(hToken);
    if (!ret) {
        ShowError();
        MessageBox(NULL, _T("创建进程失败"), _T("程序错误"), 0);
        ::exit(0);
    }
    WaitForSingleObject(pi.hProcess, INFINITE); //等待进程结束
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
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
            if (IsAdamin()) {
                OutputDebugString(L"current is run as administrator!\r\n");
               // MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
            }
            else {
                OutputDebugString(L"current is run as normal user!\r\n");
                RunAsAdmin();
                //MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
                return nRetCode;
            }
            CCommand cmd;
            ChooseAutoInvoke();
            int ret= CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，请检查网络连接！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
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
