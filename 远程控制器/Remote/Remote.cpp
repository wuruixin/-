// Remote.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
using namespace std;
#include "../common/InitSockLib.h"
#include "../common/proto.h"


BOOL RecvPackage(IN SOCKET sock, OUT PackageHeader& hdr, OUT LPBYTE& pDataBuf);
BOOL SendPackage(IN SOCKET sock, IN const PackageHeader& hdr, IN LPBYTE pDataBuf);
VOID OnC2RScreen(IN SOCKET sock);
VOID OnC2RScreenCmd(IN SOCKET sock, IN LPBYTE pDataBuf);


DWORD dwScreenWidth = 0;
DWORD dwScreenHeight = 0;

BOOL g_PictureWorking = FALSE;

int main()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        cout << "socket 失败" << endl;
        return 0;
    }

    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = htons(0x7788); //h-host to n-network s-short ntohs
    //si.sin_addr.S_un.S_addr = inet_addr("192.168.2.1");//0x0100007f;//127.0.0.1
    si.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//0x0100007f;//127.0.0.1
    int nRet = connect(sock, (sockaddr*)&si, sizeof(si));
    if (nRet == SOCKET_ERROR)
    {
        cout << "连接失败 " << endl;
        return 0;
    }

    while (true)
    {
        //接受控制端的命令
        PackageHeader hdr;
        LPBYTE pDataBuf = NULL;
        if (!RecvPackage(sock, hdr, pDataBuf))
        {
            //接受失败
            continue;
        }

        //处理命令
        switch (hdr.m_command)
        {
        case C2R_CMD:
            break;
        case C2R_SCREEN:
            OnC2RScreen(sock);
            break;
        case C2R_SCREENCMD:
            OnC2RScreenCmd(sock, pDataBuf);
            break;
        case C2R_FILETRAVELS:
            break;
        case C2R_FILEUPLOAD:
            break;
        default:
            break;
        }

    }

    std::cout << "Hello World!\n";
}

BOOL RecvPackage(IN SOCKET sock, OUT PackageHeader& hdr, OUT LPBYTE& pDataBuf)
{
    //收取包头
    int nRet = recv(sock, (char*)&hdr, sizeof(hdr), 0);
    if (nRet == 0 || nRet == SOCKET_ERROR)
    {
        return FALSE;
    }

    //申请数据的缓冲区
    if (hdr.m_dwDataLen == 0)
    {
        return TRUE;
    }

    pDataBuf = new BYTE[hdr.m_dwDataLen];
    if (pDataBuf == NULL)
    {
        return FALSE;
    }

    //循环收取数据
    int nRecvLen = 0;
    while (nRecvLen < hdr.m_dwDataLen)
    {
        nRet = recv(sock, (char*)pDataBuf + nRecvLen, hdr.m_dwDataLen - nRecvLen, 0);
        if (nRet == 0 || nRet == SOCKET_ERROR)
        {
            delete[] pDataBuf;
            return FALSE;
        }

        nRecvLen += nRet;
    }

    return TRUE;
}

BOOL SendPackage(IN SOCKET sock, IN const PackageHeader& hdr, IN LPBYTE pDataBuf)
{
    //发送包头
    int nRet = send(sock, (char*)&hdr, sizeof(hdr), 0);
    if (nRet == SOCKET_ERROR)
    {
        return FALSE;
    }
    //发送数据
    nRet = send(sock, (char*)pDataBuf, hdr.m_dwDataLen, 0);
    if (nRet == SOCKET_ERROR)
    {
        return FALSE;
    }
}

DWORD WINAPI WorkPictureThreadFunc(LPVOID lpParam) {
    SOCKET sock = (SOCKET)lpParam;
    while (g_PictureWorking)
    {
        //获取屏幕DC
        HDC hdcScreen = GetWindowDC(NULL);

        //https://www.orcode.com/question/609650_k73d79.html
        //::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
        //::SetProcessDpiAwareness(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
        //::SetProcessDPIAware();
        //::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
        
        //::SetProcessDPIAware();
        dwScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
        dwScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

        //将屏幕数据拷贝到兼容DC（兼容位图）
        HDC hDcMem = CreateCompatibleDC(hdcScreen);
        HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, dwScreenWidth, dwScreenHeight);
        SelectObject(hDcMem, hBmp);
        BitBlt(hDcMem, 0, 0, dwScreenWidth, dwScreenHeight,
            hdcScreen, 0, 0,
            SRCCOPY);

        //获取位图数据
        DWORD dwBitsOfPixel = GetDeviceCaps(hDcMem, BITSPIXEL) / 8;
        DWORD dwBufSize = dwBitsOfPixel * dwScreenHeight * dwScreenWidth + sizeof(ScreenData);
        LPBYTE pBuf = new BYTE[dwBufSize];
        if (pBuf == NULL)
        {
            return 0;
        }
        ScreenData* pSD = (ScreenData*)pBuf;
        pSD->m_dwHeight = dwScreenHeight;
        pSD->m_dwWidth = dwScreenWidth;
        GetBitmapBits(hBmp, dwBufSize - sizeof(ScreenData), pBuf + sizeof(ScreenData));

        //发送到控制端
        SendPackage(sock, PackageHeader{ R2C_SCREEN, dwBufSize }, pBuf);

        //释放
        delete[]pBuf;
        DeleteObject(hBmp);
        DeleteDC(hDcMem);
        ReleaseDC(NULL, hdcScreen);
    }
}

VOID OnC2RScreen(IN SOCKET sock)
{
    g_PictureWorking = TRUE;
    HANDLE hThread;
    hThread = CreateThread(
        NULL,                        // no security attributes 
        0,                           // use default stack size  
        WorkPictureThreadFunc,                  // thread function 
        (LPVOID)sock,                // argument to thread function 
        0,                           // use default creation flags 
        NULL);                // returns the thread identifier 
    CloseHandle(hThread);
}

VOID OnC2RScreenCmd(IN SOCKET sock, IN LPBYTE pDataBuf)
{
    ::SetProcessDPIAware();
    dwScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    dwScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    ScreenPoint* MovePoint = (ScreenPoint*)pDataBuf;
    if (MovePoint->m_pointCommand == POINT_MOVE) 
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        SetCursorPos(tempx, tempy);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_MOVE" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_LBUTTONDOWN)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_LEFTDOWN, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_LBUTTONUP)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_LEFTUP, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONUP" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_LBUTTONDBDOWN)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_LEFTDOWN, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTDOWN, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDBDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_RBUTTONDOWN)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_RIGHTDOWN, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_RBUTTONDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_RBUTTONUP)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_RIGHTUP, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDBDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_RBUTTONDBDOWN)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_RIGHTDOWN, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTUP, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTDOWN, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTUP, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDBDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_MBUTTONDOWN)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDBDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_MBUTTONUP)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_MIDDLEUP, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDBDOWN" << endl;
    }
    else if (MovePoint->m_pointCommand == POINT_MBUTTONDBDOWN)
    {
        int tempx = MovePoint->m_pointRateX * dwScreenWidth;
        int tempy = MovePoint->m_pointRateY * dwScreenHeight;
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_MIDDLEUP, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, tempx, tempy, 0, 0);
        mouse_event(MOUSEEVENTF_MIDDLEUP, tempx, tempy, 0, 0);
        cout << "x:" << tempx << "   y:" << tempy << "   POINT_LBUTTONDBDOWN" << endl;
    }
    return VOID();
}
//_DEBUG;
