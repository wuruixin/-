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

#pragma region �ṹ�嶨��
private:
/*
��
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
        WORD m_nPt; //��������
        WORD m_nLen;//���ݵĳ���
        DWORD m_nSeq; //�������
        DWORD m_nCheck;//�����ݵ�Crc32У��ֵ
        BYTE m_aryData[DATALEN];
    };
#pragma pack(pop)

    const ULONGLONG m_tmElapse = 500; //��ʱʱ��
    struct CPackageInfo
    {
        CPackageInfo() {};
        CPackageInfo(ULONGLONG tm, CPackage pkg) :m_tmLastTime(tm), m_pkg(pkg) {}

        ULONGLONG m_tmLastTime;
        CPackage m_pkg;
    };

#pragma endregion �ṹ��

    //��Ա
private:
    SOCKET m_sock;
    sockaddr_in m_siDst = {}; //�Է��ĵ�ַ
    sockaddr_in m_siSrc = {}; //�Լ��ĵ�ַ

private:
    DWORD m_nNextSendSeq = 0; //��һ�β���Ŀ�ʼ���
    DWORD m_nNextRecvSeq = 0; //��һ�δ��뻺�����İ������

private:
    map<DWORD, CPackageInfo> m_mpSend; //�洢���Ͱ�������
    CLock m_lckForSendMp; //ͬ����������m_mpSend�Ķ��߳�ͬ��
    map<DWORD, CPackage> m_mpRecv;//�洢�յ��İ�
    CLock m_lckForRecvMp; //ͬ����������m_mpRecv�Ķ��߳�ͬ��
    CByteStreamBuff m_bufRecv;//�������ݵĻ�����
    CLock m_lckForBufRecv; //ͬ����������m_bufRecv�Ķ��߳�ͬ��

private:
    HANDLE m_hSendThread = NULL;
    HANDLE m_hRecvThread = NULL;
    HANDLE m_hHandleThread = NULL;
    BOOL m_bWorking = FALSE;
    static DWORD CALLBACK SendThread(LPVOID lpParam); //�������߳�
    static DWORD CALLBACK RecvThread(LPVOID lpParam); //�հ����߳�
    static DWORD CALLBACK HandleRecvPkgsThread(LPVOID lpParam);//�������հ��������뻺�������߳�
};

