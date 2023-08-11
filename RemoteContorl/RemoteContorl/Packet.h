#pragma once
#include "pch.h"
#include "framework.h"

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {     //打包用的构造函数
		sHead = 0xFEFF;
		nLength = nSize + 4; //cmd+[]+sum
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {   //解包
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);  //一跳一字节，因为BYTE
				i += 2; //防止极端只有两个字节的包
				break;
			}
		}				//包数据可能不全，或者包头没有全部接收到
		if (i + 4 + 2 + 2 > nSize) {                                   //[][] [][][][] [][] [][][][][] [][]    //!!!!!!!!!!!!!!!
			nSize = 0; //没检出包来，用掉0字节                    //head  length   cmd            sum
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;

		if (nLength + i > nSize) {            //i 是当前包头位置，length是长度
			nSize = 0;                        //包没有完全接收到
			return;
		}
		sCmd = *(DWORD*)(pData + i); i += 2;
		if (nLength > 4)                      //读包 包长为length-cmd（2）-sum（2）
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			TRACE("%s\r\n", strData.c_str() + 12);
			i += nLength - 4;
		}
		sSum = *(DWORD*)(pData + i); i += 2;  //读sum
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {                     //和校验成功
			nSize = i; //head2 length4 data
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack)
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() { //包数据大小
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();   //指针现在指到头
		*(WORD*)pData = sHead;// *p 是指  
		pData += 2;//p是地址
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;    //用指针完成了strOut的填充

		return strOut.c_str();  //c_str: string->char

	}

public:
	WORD sHead;  //固定位 FE FF
	DWORD nLength; //包长度（从控制命令到和校验结束）
	WORD sCmd; //控制命令
	std::string strData;  //包数据
	WORD sSum;// 和校验
	std::string strOut; //整个包的数据
};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、中建
	POINT ptXY;// 坐标
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
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
}FILEINFO, * PFILEINFO;