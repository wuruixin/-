
// ControllerView.h: CControllerView 类的接口
//

#pragma once
#include "../common/InitSockLib.h"
#include "../common/proto.h"

class CControllerView : public CView
{
protected: // 仅从序列化创建
	CControllerView() noexcept;
	DECLARE_DYNCREATE(CControllerView)

private:
    SOCKET m_sockListen;
    SOCKET m_sockControl;
    BOOL m_bWorking = FALSE;
    HANDLE m_hThread;
    static DWORD CALLBACK WorkThreadFunc(LPVOID lpParam);

    BOOL RecvPackage(IN SOCKET sock, OUT PackageHeader& hdr, OUT LPBYTE& pDataBuf);
    BOOL SendPackage(IN SOCKET sock, IN const PackageHeader& hdr, IN LPBYTE pDataBuf);
    VOID OnScreen(LPBYTE pBuff, DWORD dwBufLen);

// 特性
public:
	CControllerDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 实现
public:
	virtual ~CControllerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnMnControlEnd();
    afx_msg void OnMnControlScreen();
    afx_msg void OnMnControlStart();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // ControllerView.cpp 中的调试版本
inline CControllerDoc* CControllerView::GetDocument() const
   { return reinterpret_cast<CControllerDoc*>(m_pDocument); }
#endif

