#pragma once
#include <map>
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
#include "framework.h"
#include "RemoteContorl.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include "LockDialog.h"
#include "ZHRTool.h"

class CCommand
{
public:
	CCommand();
	~CCommand() {};
	 int ExecuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)(); //��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction; //������ŵ����ܵ�ӳ��
    CLockDialog dlg;
    unsigned threadid;


protected:
    static unsigned __stdcall  threadLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }

    void threadLockDlgMain() {
        dlg.Create(IDD_DIALOG_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);
        CRect rect;                    //����
        rect.left = -10;
        rect.top = 0;       //û��
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN) * 1.10;  //w1
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
        rect.bottom = LONG(rect.bottom) * 1.10;
        //rect.right *= 1.1;
        //TRACE("right=%d bott=%d\r\r",  rect.right, rect.bottom);
        dlg.MoveWindow(rect);

        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWidth = rtText.Width();          //w0
            int x = (rect.right - nWidth) / 2;
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
        }
        //�����ö�
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        ShowCursor(false);

        //�������
        dlg.GetWindowRect(rect);
        ClipCursor(rect);
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); //����������
        //
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg:%08X wparam:%08X lparam:%08X\r\r", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x41) {   //a  �˳�
                    break;
                    TRACE("break1 threadid=%d\r\r", threadid);
                }
            }
        }
        ClipCursor(NULL);
        //�ָ����
        ShowCursor(true);
        //�ָ�������
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
        dlg.DestroyWindow();
        //_endthread();
    }

    int MakeDriverInfo() { //1->A 2->B 3->C  26->Z
        std::string result;
        for (int i = 1; i <= 26; i++)
        {
            if (_chdrive(i) == 0) {
                if (result.size() > 0)
                    result += ',';
                result += 'A' + i - 1;
            }
        }
        CPacket pack(1, (BYTE*)result.c_str(), result.size()); //ʹ������CPACKET ֱ�ӿ��԰�һ�����ݴ��
        CZHRTool::Dump((BYTE*)pack.Data(), pack.Size());
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }


    int MakeDirectoryInfo() {                  //ָ��Ŀ¼�µ��ļ����ļ���
        std::string strPath;
        std::list<FILEINFO> lstFileInfos;   //<>���� ����list<string>

        if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {     //get �� strpath����Ϊ�˰��������Ϣ
            OutputDebugString(_T("��ǰ����ǻ�ȡ�ļ��б��������ʧ�ܣ�"));
            return -1;
        }
        if (_chdir(strPath.c_str()) != 0) {   //ʧ�ܵ����
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));    //�������ǰ�finfo����ȥ��
            CServerSocket::getInstance()->Send(pack);   //TODO:����ֵ

            OutputDebugString(_T("û��Ȩ�޷���Ŀ¼��"));
            return -2;
        }
        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            FILEINFO finfo;
            finfo.HasNext = FALSE;    //��ʱszfilename=��
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));    //�������ǰ�finfo����ȥ��
            CServerSocket::getInstance()->Send(pack);   //TODO:����ֵ

            OutputDebugString(_T("û���ҵ��κ��ļ���"));
            return -3;
        }
        int count = 0; //����.��..
        do {
            FILEINFO finfo;
            finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);   //�����ȥ�ж����Ƿ�Ŀ¼�ģ���
            //finfo.IsInvalid = FALSE;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name)); //���ҵ��ļ����Ž�list��
            TRACE("%s \r\n", finfo.szFileName);

            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));    //�������ǰ�finfo����ȥ��
            CServerSocket::getInstance()->Send(pack);   //TODO:����ֵ

            count++;

            //lstFileInfos.push_back(finfo);
        } while (!_findnext(hfind, &fdata));
        TRACE("server: count=%d\r\n", count);
        //������Ϣ�����ƶ�
        FILEINFO finfo;
        finfo.HasNext = FALSE;    //��ʱszfilename=��
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));    //�������ǰ�finfo����ȥ��
        CServerSocket::getInstance()->Send(pack);   //TODO:����ֵ

        return 0;
    }

    int RunFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        CPacket pack(3, NULL, 0);    //�������ǰ�finfo����ȥ��
        CServerSocket::getInstance()->Send(pack);   //TODO:����ֵ
        return 0;
    }

    int DownloadFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);   //����ֻ��Ϊ����ʱ������Щͷ����ʱ����Щ���ɴ�����������
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");   //�ϴ���ȥ
        if (err != 0) {
            CPacket pack(4, (BYTE*)data, 8);   //��data����ȥ ��ʾ�ļ�����
            CServerSocket::getInstance()->Send(pack);
            return -1;
        }
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END);
            data = _ftelli64(pFile);
            CPacket head(4, (BYTE*)&data, 8); //�õ���������
            CServerSocket::getInstance()->Send(head);
            fseek(pFile, 0, SEEK_SET);

            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                CPacket pack(4, (BYTE*)buffer, rlen);
                CServerSocket::getInstance()->Send(pack);
            } while (rlen >= 1024);
            fclose(pFile);
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MouseEvent() {

        MOUSEEV mouse;
        if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
            DWORD nFlags = 0;
            switch (mouse.nButton) {
            case 0:  //left
                nFlags = 1;
                break;
            case 1:   //right
                nFlags = 2;
                break;
            case 2:   //middle
                nFlags = 4;
                break;
            case 4:   //no move
                nFlags = 8;
            }
            if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction) {
            case 0://���
                nFlags |= 0x10;
                break;
            case 1://˫��
                nFlags |= 0x20;
                break;
            case 2://����
                nFlags |= 0x40;
                break;
            case 3://�ſ�
                nFlags |= 0x80;
                break;
            default:
                break;
            }
            TRACE("Mouse event:%08X x:%d y:%d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlags)
            {
            case 0x21://���˫��
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11:  //�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://����ɿ�
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;//#####################
            case 0x22://�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://�Ҽ��ɿ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;//########################
            case 0x24://�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://�м��ɿ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08:
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break; //�����ƶ�
            }
            CPacket pack(4, NULL, 0);
            CServerSocket::getInstance()->Send(pack);

        }

        else {
            OutputDebugString(_T("��ȡ����������ʧ�ܣ�"));
            return -1;
        }

        return 0;
    }

    int SendScreen()
    {
        CImage screen; //GDI
        HDC hScreen = ::GetDC(NULL);
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel);
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY); //dpi-for find
        ReleaseDC(NULL, hScreen);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL) return -1;
        IStream* pStream = NULL; //�ڴ���
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK)
        {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);   //�����������浽�ڴ棻�����Ǳ��浽�ļ�
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)GlobalLock(hMem);
            SIZE_T nSize = GlobalSize(hMem);
            CPacket pack(6, pData, nSize);
            CServerSocket::getInstance()->Send(pack);
            GlobalLock(hMem);
        }
        // screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();
        return 0;
    }

    int LockMachine() {
        if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {  //dlgΪ�ջ��߱����ٹ�����ȥ����
           // _beginthread(threadLockDlg, 0, NULL);  
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
        }
        CPacket pack(7, NULL, 0);
        CServerSocket::getInstance()->Send(pack);

        return 0;
    }

    int UnlockMachine() {
        //dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
        CPacket pack(8, NULL, 0);
        CServerSocket::getInstance()->Send(pack);

        return 0;
    }



    int TestConnect() {
        CPacket pack(1981, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("SERVER TESTCONNECT: Send ret= %d\r\n", ret);
        return 0;
    }

    int DeleteLocalFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        TCHAR sPath[MAX_PATH] = _T("");
        //mbstowcs(sPath, strPath.c_str(), strPath.size());
        MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
            sizeof(sPath) / sizeof(TCHAR));
        DeleteFileA(strPath.c_str());

        CPacket pack(9, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("Send ret= %d\r\n", ret);
        return 0;
    }
};

