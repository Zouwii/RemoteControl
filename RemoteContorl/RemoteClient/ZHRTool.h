#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>
class CZHRTool
{
public:
    static void Dump(BYTE* pData, size_t nSize)      //导出一下看看
    {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++)
        {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);   //%02X  16进制输出，宽度为2
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }
    static int Bytes2Image(CImage& image, std::string& strBuffer) {
		BYTE* pData = (BYTE*)strBuffer.c_str(); //放缓存
		//与REMOTECONTROL-SENDSCREEN 反过来
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL)
		{
			TRACE("内存不足了！\r\n");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL) image.Destroy();
			image.Load(pStream);
		}
		return hRet;
    }
};

