#pragma once
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#include <map>
using namespace std;

#include "ByteStreamBuff.h"
#include "CLock.h"
#include "CCrc32.h"

class CUMT
{
public:
    BOOL Accept(LPCTSTR szIp, USHORT nPort);


    BOOL Connect(LPCTSTR szIp, USHORT nPort);
    DWORD Send(LPBYTE pBuff, DWORD dwBufLen);
    DWORD Recv(LPBYTE pBuff, DWORD dwBufLen);
    VOID Close();

private:
    BOOL Init();
    VOID Clear();
    VOID Log(const char* szFmt, ...);

#pragma region 结构体定义
private:
/*
包
*/
#define DATALEN 1460
    enum PackageType {PT_SYN = 4, PT_ACK = 2, PT_DATA = 1,  PT_FIN };

#pragma pack(push)
#pragma pack(1)
    struct CPackage
    {
        CPackage() {}
        CPackage(WORD pt, DWORD dwSeq, LPBYTE pData = NULL, WORD nDataLen=0)
        {
            m_nPt = pt;
            m_nSeq = dwSeq;
            m_nLen = 0;
            m_nCheck = 0;
            if (pData != NULL)
            {
                memcpy(m_aryData, pData, nDataLen);
                m_nLen = nDataLen;
                m_nCheck = CCrc32::crc32(pData, nDataLen);
            }
        }
        WORD m_nPt; //包的类型
        WORD m_nLen;//数据的长度
        DWORD m_nSeq; //包的序号
        DWORD m_nCheck;//包数据的Crc32校验值
        BYTE m_aryData[DATALEN];
    };
#pragma pack(pop)

    const ULONGLONG m_tmElapse = 500; //超时时间
    struct CPackageInfo
    {
        CPackageInfo() {};
        CPackageInfo(ULONGLONG tm, CPackage pkg) :m_tmLastTime(tm), m_pkg(pkg) {}

        ULONGLONG m_tmLastTime;
        CPackage m_pkg;
    };

#pragma endregion 结构体

    //成员
private:
    SOCKET m_sock;
    sockaddr_in m_siDst = {}; //对方的地址
    sockaddr_in m_siSrc = {}; //自己的地址

private:
    DWORD m_nNextSendSeq = 0; //下一次拆包的开始序号
    DWORD m_nNextRecvSeq = 0; //下一次存入缓冲区的包的序号

private:
    map<DWORD, CPackageInfo> m_mpSend; //存储发送包的容器
    CLock m_lckForSendMp; //同步对象，用于m_mpSend的多线程同步
    map<DWORD, CPackage> m_mpRecv;//存储收到的包
    CLock m_lckForRecvMp; //同步对象，用于m_mpRecv的多线程同步
    CByteStreamBuff m_bufRecv;//接受数据的缓冲区
    CLock m_lckForBufRecv; //同步对象，用于m_bufRecv的多线程同步

private:
    HANDLE m_hSendThread = NULL;
    HANDLE m_hRecvThread = NULL;
    HANDLE m_hHandleThread = NULL;
    BOOL m_bWorking = FALSE;
    static DWORD CALLBACK SendThread(LPVOID lpParam); //发包的线程
    static DWORD CALLBACK RecvThread(LPVOID lpParam); //收包的线程
    static DWORD CALLBACK HandleRecvPkgsThread(LPVOID lpParam);//将包从收包容器放入缓冲区的线程
};

