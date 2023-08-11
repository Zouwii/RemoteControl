#pragma once
#include "pch.h"
#include "framework.h"

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {     //����õĹ��캯��
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
	CPacket(const BYTE* pData, size_t& nSize) {   //���
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);  //һ��һ�ֽڣ���ΪBYTE
				i += 2; //��ֹ����ֻ�������ֽڵİ�
				break;
			}
		}				//�����ݿ��ܲ�ȫ�����߰�ͷû��ȫ�����յ�
		if (i + 4 + 2 + 2 > nSize) {                                   //[][] [][][][] [][] [][][][][] [][]    //!!!!!!!!!!!!!!!
			nSize = 0; //û����������õ�0�ֽ�                    //head  length   cmd            sum
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;

		if (nLength + i > nSize) {            //i �ǵ�ǰ��ͷλ�ã�length�ǳ���
			nSize = 0;                        //��û����ȫ���յ�
			return;
		}
		sCmd = *(DWORD*)(pData + i); i += 2;
		if (nLength > 4)                      //���� ����Ϊlength-cmd��2��-sum��2��
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			TRACE("%s\r\n", strData.c_str() + 12);
			i += nLength - 4;
		}
		sSum = *(DWORD*)(pData + i); i += 2;  //��sum
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {                     //��У��ɹ�
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
	int Size() { //�����ݴ�С
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();   //ָ������ָ��ͷ
		*(WORD*)pData = sHead;// *p ��ָ  
		pData += 2;//p�ǵ�ַ
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;    //��ָ�������strOut�����

		return strOut.c_str();  //c_str: string->char

	}

public:
	WORD sHead;  //�̶�λ FE FF
	DWORD nLength; //�����ȣ��ӿ��������У�������
	WORD sCmd; //��������
	std::string strData;  //������
	WORD sSum;// ��У��
	std::string strOut; //������������
};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����н�
	POINT ptXY;// ����
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	char szFileName[256];
	BOOL IsDirectory; //�Ƿ�Ŀ¼ 0��1�� WENJIANJIA
	BOOL IsInvalid; // �Ƿ���Ч
	BOOL HasNext;  //0�� 1��
}FILEINFO, * PFILEINFO;