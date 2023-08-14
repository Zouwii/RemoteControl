#pragma once

#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "ZHRTool.h"

#define WM_SEND_PACK (WM_USER+1) //发送包数据
#define WM_SEND_DATA (WM_USER+2) //发送数据
#define WM_SHOW_STATUS (WM_USER+3) //展示状态
#define WM_SHOW_WATCH (WM_USER+4) //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)//自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitController();
	//启动
	int Invoke(CWnd*& pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);
	//更新网络服务器的地址
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(pack);
	}
	//1 查看分区
	//2 查看指定目录下的文件 
	//3 打开文件
	//4 下载文件 
	//9 删除文件
	//5 鼠标操作
	//6 屏幕监控
	//7 锁定
	//8 解锁
	//1981 测试连接
	//SendCommand返回值是命令号 返回负数为错误
	int SendCommandPacket(
		int nCmd, 
		bool bAutoClose=true, 
		BYTE* pData=NULL,
		size_t nLength=0)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(CPacket(nCmd,pData,nLength));
		int cmd =DealCommand();
		TRACE("CLIENT ACK: %d \r\n", cmd);
		if (bAutoClose)CloseSocket();
		return cmd;
	}
	int GetImage(CImage& image) {
		//更新数据到缓存区
		CClientSocket* pClient = CClientSocket::getInstance();
		return CZHRTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	int DownFile(CString strPath) {
		CFileDialog dlg(FALSE, "*", strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
			NULL, &m_remoteDlg);
		if (dlg.DoModal() == IDOK) {
			m_strRemote = strPath;
			m_strLocal = dlg.GetPathName();


			m_hThreadDownload=(HANDLE) _beginthread
			(&CClientController::threadDownloadEntry, 0, this);
			if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
				return -1;
			}
			m_statusDlg.BeginWaitCursor(); //光标
			m_statusDlg.m_info.SetWindowTextA(_T("命令正在执行中！"));
			m_statusDlg.ShowWindow(SW_SHOW);
			m_statusDlg.CenterWindow(&m_remoteDlg);
			m_statusDlg.SetActiveWindow();
		}
		return 0;	
	}

	void StartWatchScreen();

protected:
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);

	CClientController() :
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg) //指定父窗口
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_isClosed = true;
		m_nThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);//等线程执行完了
	}
	void threadFunc();//真正的线程函数
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}

	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);


private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	} MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC)
		(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC>m_mapFunc;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	CString m_strRemote; //下载文件的远程路径
	CString m_strLocal;  //下载文件的本地保存路径
	bool m_isClosed;//监视是否关闭
	unsigned m_nThreadID;
	static CClientController* m_instance;

	class CHelper {
	public:
		CHelper()
		{
			CClientController::getInstance();
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;

};

