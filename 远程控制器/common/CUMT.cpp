#include "CUMT.h"
#include <time.h>
#pragma omp parallel shared


BOOL CUMT::Accept(LPCTSTR szIp, USHORT nPort)
{
    //������
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET)
    {
        return FALSE;
    }

    sockaddr_in siServer = {};
    siServer.sin_family = AF_INET;
    siServer.sin_addr.S_un.S_addr = inet_addr((char*)szIp);
    siServer.sin_port = htons(nPort);
    int nRet = bind(m_sock, (sockaddr*)&siServer, sizeof(siServer));
    if (nRet == SOCKET_ERROR)
    {
        closesocket(m_sock);
        return FALSE;
    }

    while (true)
    {
        //�յ�һ����
        CPackage pkg;
        int nLen = sizeof(m_siDst);
        nRet = recvfrom(m_sock, (char*)&pkg, sizeof(pkg), 0, (sockaddr*)&m_siDst, &nLen);
        if (nRet == 0 || nRet == SOCKET_ERROR || pkg.m_nPt != PT_SYN || pkg.m_nSeq != m_nNextRecvSeq)
        {
            continue;
        }

        //�ص�һ��
        CPackage pkgSend(PT_SYN|PT_ACK, m_nNextSendSeq);
        nRet = sendto(m_sock, (char*)&pkgSend, sizeof(pkgSend), 0, (sockaddr*)&m_siDst, sizeof(m_siDst));
        if (nRet == SOCKET_ERROR)
        {
            continue;
        }

        //�յڶ�����
        nRet = recvfrom(m_sock, (char*)&pkg, sizeof(pkg), 0, (sockaddr*)&m_siDst, &nLen);
        if (nRet == 0 || nRet == SOCKET_ERROR || pkg.m_nPt != PT_ACK || pkg.m_nSeq != m_nNextRecvSeq)
        {
            continue;
        }

        //���ӽ���
        break;
    }

    //��ʼ��
    return Init();
}

BOOL CUMT::Init()
{
    m_nNextRecvSeq++;
    m_nNextSendSeq++;

    m_bWorking = TRUE;
    m_hSendThread = CreateThread(NULL, 0, SendThread, this, 0, NULL);
    if (m_hSendThread == NULL)
    {
        Clear();
        return FALSE;
    }
    //�������������ݵ��߳�
    m_hRecvThread = CreateThread(NULL, 0, RecvThread, this, 0, NULL);
    if (m_hRecvThread == NULL)
    {
        Clear();
        return FALSE;
    }
    m_hRecvThread = CreateThread(NULL, 0, RecvThread, this, 0, NULL);
    if (m_hRecvThread == NULL)
    {
        Clear();
        return FALSE;
    }
    m_hHandleThread = CreateThread(NULL, 0, HandleRecvPkgsThread, this, 0, NULL);
    if (m_hHandleThread == NULL)
    {
        Clear();
        return FALSE;
    }
    return TRUE;
}

BOOL CUMT::Connect(LPCTSTR szIp, USHORT nPort)
{
    //������
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET)
    {
        return FALSE;
    }

    
    m_siDst.sin_family = AF_INET;
    m_siDst.sin_addr.S_un.S_addr = inet_addr((char*)szIp);
    m_siDst.sin_port = htons(nPort);
    //����һ��
    CPackage pkgSend(PT_SYN , m_nNextSendSeq);
    int nRet = sendto(m_sock, (char*)&pkgSend, sizeof(pkgSend), 0, (sockaddr*)&m_siDst, sizeof(m_siDst));
    if (nRet == SOCKET_ERROR)
    {
        Clear();
        return FALSE;
    }

    //�յ�һ����
    CPackage pkg;
    int nLen = sizeof(m_siDst);
    nRet = recvfrom(m_sock, (char*)&pkg, sizeof(pkg), 0, (sockaddr*)&m_siDst, &nLen);
    if (nRet == 0 || nRet == SOCKET_ERROR || pkg.m_nPt != (PT_SYN|PT_ACK) || pkg.m_nSeq != m_nNextRecvSeq)
    {
        Clear();
        return FALSE;
    }

    //�յڶ�����
    CPackage pkgSendAck(PT_ACK, m_nNextSendSeq);
    nRet = sendto(m_sock, (char*)&pkgSendAck, sizeof(pkgSendAck), 0, (sockaddr*)&m_siDst, sizeof(m_siDst));
    if (nRet == SOCKET_ERROR)
    {
        Clear();
        return FALSE;
    }
    
    //���ӽ���
    return Init();
}

DWORD CUMT::Send(LPBYTE pBuff, DWORD dwBufLen)
{
    //���Ʒ��������еİ����Ϊ100��
    while (true)
    {
        m_lckForSendMp.Lock();
        DWORD dwSize = m_mpSend.size();
        m_lckForSendMp.UnLock();
        if (dwSize > 100)
        {
            Sleep(1);
            continue;
        }
        else
        {
            break;
        }
    }

    //��� , ��������
    m_lckForSendMp.Lock();
    DWORD dwCnt = ( dwBufLen % DATALEN == 0 ? dwBufLen / DATALEN : (dwBufLen / DATALEN + 1));

    for (DWORD i = 0;i < dwCnt; ++i)
    {
        DWORD dwLen = DATALEN;
        if (i == dwCnt - 1)
        {
            dwLen = (dwBufLen - i * DATALEN);
        }
        CPackage pkg(PT_DATA, m_nNextSendSeq, pBuff + i * DATALEN, dwLen);
        m_mpSend[m_nNextSendSeq] = CPackageInfo(0, pkg);

        Log("[umt]: package ==> map seq:%d", m_nNextSendSeq);
        ++m_nNextSendSeq;
    }
    m_lckForSendMp.UnLock();

    return dwBufLen;
}

DWORD CUMT::Recv(LPBYTE pBuff, DWORD dwBufLen)
{
    while (true)
    {
        m_lckForBufRecv.Lock();
        DWORD dwSize = m_bufRecv.GetSize();
        m_lckForBufRecv.UnLock();
        if (dwSize <= 0)
        {
            //û���ݣ�����
            Sleep(1);
        }
        else
        {
            //������
            break;
        }
    }


    //������
    m_lckForBufRecv.Lock();
    DWORD dwDataLen = (m_bufRecv.GetSize() > dwBufLen) ? dwBufLen : m_bufRecv.GetSize();
    m_bufRecv.Read(pBuff, dwDataLen);
    m_lckForBufRecv.UnLock();

    return dwDataLen;
}

VOID CUMT::Close()
{

}

VOID CUMT::Clear()
{
    closesocket(m_sock);
    m_bWorking = FALSE;
    if (m_hSendThread != NULL)
    {
        CloseHandle(m_hSendThread);
    }
    if (m_hRecvThread != NULL)
    {
        CloseHandle(m_hRecvThread);
    }
    if (m_hHandleThread != NULL)
    {
        CloseHandle(m_hHandleThread);
    }
}

VOID CUMT::Log(const char* szFmt, ...)
{
    char szBuff[MAXWORD] = {};
    va_list vl;
    va_start(vl, szFmt);
    vsprintf(szBuff, szFmt, vl);
    va_end(vl);
    OutputDebugString(szBuff);
}

DWORD CALLBACK CUMT::SendThread(LPVOID lpParam)
{
    CUMT* pThis = (CUMT*)lpParam;
    while (pThis->m_bWorking)
    {
        pThis->m_lckForSendMp.Lock();
        for (auto& pi: pThis->m_mpSend)
        {
            //û�з���ȥ�İ�������
            //��ʱ�İ�������
            ULONGLONG tmCurrent = GetTickCount64();
            if (tmCurrent - pi.second.m_tmLastTime > pThis->m_tmElapse)
            {
                if (pi.second.m_tmLastTime == 0)
                {
                    pThis->Log("[umt]: package ==> net first   seq:%d", pi.second.m_pkg.m_nSeq);
                }
                else
                {
                    pThis->Log("[umt]: package ==> net timeout seq:%d", pi.second.m_pkg.m_nSeq);
                }

                sendto(
                    pThis->m_sock, 
                    (char*)&pi.second.m_pkg, sizeof(pi.second.m_pkg), 
                    0, 
                    (sockaddr*)&pThis->m_siDst, sizeof(pThis->m_siDst));
                pi.second.m_tmLastTime = tmCurrent;
            }
        }
        pThis->m_lckForSendMp.UnLock();

        //�г��߳�
        Sleep(1);
    }
    return 0;
}

DWORD CALLBACK CUMT::RecvThread(LPVOID lpParam)
{
    CUMT* pThis = (CUMT*)lpParam;
    while (pThis->m_bWorking)
    {
        sockaddr_in si = {};
        int nLen = sizeof(si);
        CPackage pkg;
        int nRet = recvfrom(pThis->m_sock,
            (char*)&pkg, sizeof(pkg),
            0,
            (sockaddr*)&si, &nLen);
        if (nRet == 0 || nRet == SOCKET_ERROR)
        {
            continue;
        }

        //�жϰ�������
        switch (pkg.m_nPt)
        {
        case PT_ACK: 
        {
            pThis->m_lckForSendMp.Lock();
            pThis->Log("[umt]: package ==> ack seq:%d", pkg.m_nSeq);
            pThis->m_mpSend.erase(pkg.m_nSeq);
            pThis->m_lckForSendMp.UnLock();
            break;
        }

        case PT_DATA:
        {
            //У��
            DWORD dwCheck = CCrc32::crc32(pkg.m_aryData, pkg.m_nLen);

            //У��ʧ��
            if (dwCheck != pkg.m_nCheck)
            {
                //������
                break;
            }

            //У��ɹ�, �ظ�ack
            CPackage pkgAck(PT_ACK, pkg.m_nSeq);
            sendto(pThis->m_sock,
                (char*)&pkgAck, sizeof(pkgAck),
                0,
                (sockaddr*)&pThis->m_siDst, sizeof(pThis->m_siDst));
            //��������
            pThis->m_lckForRecvMp.Lock();
            if (pThis->m_mpRecv.find(pkg.m_nSeq) != pThis->m_mpRecv.end()//�����д���ŵİ��Ѿ�����
                || pkg.m_nSeq < pThis->m_nNextRecvSeq)//����ŵİ��е������Ѿ����뻺����
            {
                pThis->m_lckForRecvMp.UnLock();
                break;
            }
            pThis->m_mpRecv[pkg.m_nSeq] = pkg;
            pThis->Log("[umt]: package net ==> map seq:%d", pkg.m_nSeq);
            pThis->m_lckForRecvMp.UnLock();

            break;
        }
        default:
            break;
        }
    }
    return 0;
}

DWORD CALLBACK CUMT::HandleRecvPkgsThread(LPVOID lpParam)
{
    CUMT* pThis = (CUMT*)lpParam;
    while (pThis->m_bWorking)
    {
        pThis->m_lckForBufRecv.Lock();
        while (true)
        {
            pThis->m_lckForRecvMp.Lock();
            if (pThis->m_mpRecv.find(pThis->m_nNextRecvSeq) != pThis->m_mpRecv.end())
            {
                //����������
                auto& pkg = pThis->m_mpRecv[pThis->m_nNextRecvSeq];
                pThis->m_bufRecv.Write(pkg.m_aryData, pkg.m_nLen);

                pThis->Log("[umt]: package ==> buf seq:%d", pThis->m_nNextRecvSeq);

                //���������Ƴ���
                pThis->m_mpRecv.erase(pThis->m_nNextRecvSeq);

                //��Ÿ���
                ++pThis->m_nNextRecvSeq;
            }
            else
            {
                pThis->m_lckForRecvMp.UnLock();
                break;
            }
            pThis->m_lckForRecvMp.UnLock();
        }
        pThis->m_lckForBufRecv.UnLock();

        //�г���ǰ�߳�
        Sleep(1);
    }
    return 0;
}

