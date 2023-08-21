#pragma once
class CZHRTool
{
public:
    static void Dump(BYTE* pData, size_t nSize)      //����һ�¿���
    {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++)
        {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);   //%02X  16������������Ϊ2
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

    static bool IsAdamin() {
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
        //������˵������Ȩ��
        printf("length of tokeninformation is %d\r\n", len);
        return false;
    }

    static  bool RunAsAdmin()
    {//��ȡ����ԱȨ�ޣ�ʹ�ø�Ȩ�޴�������
        //���ز����� ����administrator�˻� ��ֹ�������¼���ؿ���̨
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
        if (!ret) {
            ShowError();
            MessageBox(NULL, sPath, _T("�������"), 0); //TODO:ȥ����������
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE); //�ȴ����̽���
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    static void ShowError() {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno);//��׼c���Կ�
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf, 0, NULL);   //GETLASTERRORֻ��ʾ��ǰ�̴߳���
        OutputDebugString(lpMessageBuf);
        LocalFree(lpMessageBuf);
    }

    static bool WriteRegisterTable(const CString& strPath)
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
            MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n ��������ʧ�ܣ�"), _T("����"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
        ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n ��������ʧ�ܣ�"), _T("����"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
        RegCloseKey(hKey);
        return true;
    }

    static bool Init()
    {//���ڴ�MFC��������Ŀ��ʼ����ͨ�ã�
        HMODULE hModule = ::GetModuleHandle(nullptr);
        if (hModule == nullptr) {
            wprintf(L"����: GetModuleHandle ʧ��\n");
            return false;
        }
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            wprintf(L"����: MFC ��ʼ��ʧ��\n");
            return false;
        }
        return true;
    }

};

