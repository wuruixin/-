
// ControllerView.cpp: CControllerView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "Controller.h"
#endif

#include "ControllerDoc.h"
#include "ControllerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CControllerView

IMPLEMENT_DYNCREATE(CControllerView, CView)

BEGIN_MESSAGE_MAP(CControllerView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
    ON_COMMAND(MN_CONTROL_END, &CControllerView::OnMnControlEnd)
    ON_COMMAND(MN_CONTROL_SCREEN, &CControllerView::OnMnControlScreen)
    ON_COMMAND(MN_CONTROL_START, &CControllerView::OnMnControlStart)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_MBUTTONUP()
    ON_WM_MBUTTONDOWN()
    ON_WM_MBUTTONDBLCLK()
END_MESSAGE_MAP()

// CControllerView 构造/析构

CControllerView::CControllerView() noexcept
{
	// TODO: 在此处添加构造代码

}

CControllerView::~CControllerView()
{
}

BOOL CControllerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CControllerView 绘图

void CControllerView::OnDraw(CDC* /*pDC*/)
{
	CControllerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
}


// CControllerView 打印

BOOL CControllerView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备
	return DoPreparePrinting(pInfo);
}

void CControllerView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CControllerView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}


// CControllerView 诊断

#ifdef _DEBUG
void CControllerView::AssertValid() const
{
	CView::AssertValid();
}

void CControllerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CControllerDoc* CControllerView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CControllerDoc)));
	return (CControllerDoc*)m_pDocument;
}
#endif //_DEBUG


// CControllerView 消息处理程序

//建立链接，不断接收数据，根据命令执行相应的函数
DWORD CALLBACK CControllerView::WorkThreadFunc(LPVOID lpParam)
{
    CControllerView* pThis = (CControllerView*)lpParam;

#pragma region 初始化
    pThis->m_sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pThis->m_sockListen == INVALID_SOCKET)
    {
       AfxMessageBox("socket 创建失败");
        return 0;
    }

    sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = htons(0x7788); //h-host to n-network s-short ntohs
    si.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");//0x0100007f;//127.0.0.1
    int nRet = bind(pThis->m_sockListen, (sockaddr*)&si, sizeof(si));
    if (nRet == SOCKET_ERROR)
    {
        AfxMessageBox("bind 失败");
        return 0;
    }

    nRet = listen(pThis->m_sockListen, 1);
    if (nRet == SOCKET_ERROR)
    {
        AfxMessageBox("listen 失败");
        return 0;
    }
#pragma endregion 初始化

    sockaddr_in siControl;
    int nLen = sizeof(siControl);
    pThis->m_sockControl = accept(pThis->m_sockListen, (sockaddr*)&siControl, &nLen);
    if (pThis->m_sockControl == INVALID_SOCKET)
    {
        AfxMessageBox("连接失败");
        return 0;
    }

    //如果处于工作状态
    while (pThis->m_bWorking)
    {
        PackageHeader hdr;
        LPBYTE pBuff = NULL;
        if (!pThis->RecvPackage(pThis->m_sockControl, hdr, pBuff))
        {
            continue;
        }

        switch (hdr.m_command)
        {
        case R2C_SCREEN:
        {
            pThis->OnScreen(pBuff, hdr.m_dwDataLen);
            delete[] pBuff;
            break;
        }
        default:
            delete[] pBuff;
            break;
        }
    }
}

BOOL CControllerView::RecvPackage(IN SOCKET sock, OUT PackageHeader & hdr, OUT LPBYTE & pDataBuf)
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

//发送包
//参数1：待发送的socket
//参数2：发送的包头
//参数3：发送的数据
BOOL CControllerView::SendPackage(IN SOCKET sock, IN const PackageHeader & hdr, IN LPBYTE pDataBuf)
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
    return TRUE;
}

//接收屏幕消息后，进行消息处理
VOID CControllerView::OnScreen(LPBYTE pBuff, DWORD dwBufLen)
{
    //将数据栈转为ScreenData
    ScreenData* pSD = (ScreenData*)pBuff;
    LPBYTE pScreenData = pBuff + sizeof(ScreenData);

    CClientDC dcClient(this);
    CDC dcMem;
    dcMem.CreateCompatibleDC(&dcClient);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dcClient, pSD->m_dwWidth, pSD->m_dwHeight);
    DWORD dwDataSize = dwBufLen - sizeof(ScreenData);
    bmp.SetBitmapBits(dwDataSize, pScreenData);
    dcMem.SelectObject(&bmp);

    CRect tClient;
    GetClientRect(&tClient);

    dcClient.SetStretchBltMode(HALFTONE);
    dcClient.StretchBlt(0, 0, tClient.Width(), tClient.Height(),
        &dcMem, 0, 0, pSD->m_dwWidth, pSD->m_dwHeight,
        SRCCOPY);
    /*dcClient.BitBlt(0, 0, pSD->m_dwWith, pSD->m_dwHeight,
        &dcMem, 0, 0,
        SRCCOPY);*/
}

//启动工作线程
void CControllerView::OnMnControlStart()
{
    m_bWorking = TRUE;
    m_hThread = CreateThread(NULL, 0, WorkThreadFunc, this, 0, NULL);
    if (m_hThread == NULL)
    {
        AfxMessageBox("工作线程启动失败");
        return;
    }
}

//结束工作线程
void CControllerView::OnMnControlEnd()
{

}

//监视屏幕信息
void CControllerView::OnMnControlScreen()
{
    PackageHeader hdr{ C2R_SCREEN, 0 };
    SendPackage(m_sockControl, hdr, NULL);
}



void CControllerView::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_MOVE };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }

    CView::OnMouseMove(nFlags, point);
}


void CControllerView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_LBUTTONDOWN };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }

    CView::OnLButtonDown(nFlags, point);
}


void CControllerView::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_LBUTTONUP };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }

    CView::OnLButtonUp(nFlags, point);
}


void CControllerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_LBUTTONDBDOWN };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }

    CView::OnLButtonDblClk(nFlags, point);
}


void CControllerView::OnRButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_RBUTTONDOWN };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }
    CView::OnRButtonDown(nFlags, point);
}


void CControllerView::OnRButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_RBUTTONUP };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }
    CView::OnRButtonUp(nFlags, point);
}


void CControllerView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_RBUTTONDBDOWN };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }
    CView::OnRButtonDblClk(nFlags, point);
}


void CControllerView::OnMButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_MBUTTONDOWN };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }
    CView::OnMButtonDown(nFlags, point);
}

void CControllerView::OnMButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_MBUTTONUP };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }
    CView::OnMButtonUp(nFlags, point);
}



void CControllerView::OnMButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    if (m_bWorking) {
        CRect tClient;
        GetClientRect(&tClient);
        double tempx = (double)point.x / (double)tClient.Width();
        double tempY = (double)point.y / (double)tClient.Height();
        ScreenPoint nMovePoint = { tempx,tempY,POINT_MBUTTONDBDOWN };
        PackageHeader hdr{ C2R_SCREENCMD, sizeof(nMovePoint) };
        SendPackage(m_sockControl, hdr, (LPBYTE)&nMovePoint);
    }
    CView::OnMButtonDblClk(nFlags, point);
}
