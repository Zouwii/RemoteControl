#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"
//��̬������ͷ������ Ҫcpp��ʵ��
std::map<UINT, CClientController::MSGFUNC>
CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;


CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
			{WM_SHOW_STATUS,&CClientController::OnShowStatus} ,
			{WM_SHOW_WATCH,&CClientController::OnShowWatcher},
			{(UINT)-1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0,
		&CClientController::threadEntry,
		this, 0, &m_nThreadID);//CreateThread
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL) return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
		(WPARAM)&msg, (LPARAM)&hEvent);
	WaitForSingleObject(hEvent, INFINITE);//�����޾�ȥ��
	CloseHandle(hEvent);
	return info.result;
}

int CClientController::SendCommandPacket(int nCmd, bool bAutoClose,
	BYTE* pData, size_t nLength, std::list<CPacket>* plstPacks)
{
	TRACE("cmd %d %s start %11d\r\n", nCmd,__FUNCTION__, GetTickCount64());
	CClientSocket* pClient = CClientSocket::getInstance();
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	//��Ӧ��ֱ�ӷ��ͣ�Ҫ��������ȥ
	std::list<CPacket>lstPacks;
	if (plstPacks == NULL)
		plstPacks = &lstPacks;
	pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvent),*plstPacks,bAutoClose);

	CloseHandle(hEvent);//�����¼��������ֹ��Դ�ľ�
	if (plstPacks->size() > 0) {
		TRACE("%s start %11d\r\n", __FUNCTION__, GetTickCount64());
		return plstPacks->front().sCmd;
	}
	TRACE("%s start %d\r\n", __FUNCTION__, GetTickCount64());
	return -1;
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, "*", strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();


		m_hThreadDownload = (HANDLE)_beginthread
		(&CClientController::threadDownloadEntry, 0, this);
		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
			return -1;
		}
		m_statusDlg.BeginWaitCursor(); //���
		m_statusDlg.m_info.SetWindowTextA(_T("��������ִ���У�"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	//CWatchDialog dlg(&m_remoteDlg);
	m_hThreadWatch = (HANDLE)_beginthread(
		CClientController::threadWatchScreenEntry, 0, this);
	//dlg.DoModal();
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed) { //==while(true)
		if (m_watchDlg.isFull()==false) {
			std::list<CPacket>lstPacks;
			int ret = SendCommandPacket(6,true,NULL,0,&lstPacks);
			if (ret == 6)
			{
				if (CZHRTool::Bytes2Image(m_watchDlg.GetImage(), lstPacks.front().strData) == 0) {
					m_watchDlg.SetImageStatus(true); //m_isFull->true
					TRACE("�ɹ�����ͼƬ\r\n");
				}

				else {
					TRACE("��ȡͼƬʧ��! %d\r\n", ret);
				}
			}
		}
		else {
			Sleep(1);
		}
	}
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile == NULL) {
		AfxMessageBox(_T("û��Ȩ�ޱ�����ļ��������ļ��޷�����"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	do {
		int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)m_strRemote,
			m_strRemote.GetLength());
		if (ret < 0)
		{
			AfxMessageBox(_T("ִ����������ʧ�ܣ�"));
			TRACE("download failed! ret=%d\r\n", ret);
			break;
		}
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nLength == 0) {
			AfxMessageBox(_T("�ļ�����Ϊ0���޷���ȡ�ļ���"));
			break;
		}
		long long nCount = 0;
		while (nCount < nLength) {
			pClient->DealCommand();
			if (ret < 0)
			{
				AfxMessageBox(_T("����ʧ�ܣ�"));
				TRACE("����ʧ�� ret=%d\r\n", ret);
				break;
			}
			pClient->GetPacket().strData.c_str();
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}

	} while (false);
	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {//������Ϣѭ���߳�   
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it =
				m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message,
					pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}



LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();//�򿪴���
}
