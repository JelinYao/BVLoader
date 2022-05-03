#include "pch.h"
#include "wnd_base.h"
#include "soft_define.h"
#include "resource.h"

DUI_BEGIN_MESSAGE_MAP(WndBase, CNotifyPump)
DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK, OnNotifyClick)
DUI_END_MESSAGE_MAP()
WndBase::WndBase()
    : m_dwStyle(UI_WNDSTYLE_FRAME)
    , m_dwExStyle(0)
    , m_bShowOnTaskbar(true)
    , m_bShowShadow(false)
    , m_bEscape(false)
    , m_bRoundRect(false)
    , m_bDeleteThis(false)
{

}

WndBase::~WndBase(void)
{

}

LPCTSTR WndBase::GetWindowClassName()const
{
    return kAppWindowClassName;
}

void WndBase::OnFinalMessage(HWND hWnd)
{
    m_pm.RemoveNotifier(this);
    if (m_bDeleteThis) {
        delete this;
    }
}

HWND WndBase::Create(HWND hParentWnd, bool child/* = false*/, int nPosX /*= 0*/, int nPosY /*= 0*/) {
    if (m_bShowOnTaskbar) {
        m_dwExStyle = m_dwExStyle | WS_EX_STATICEDGE | WS_EX_APPWINDOW;
    }
    else {
        m_dwExStyle = m_dwExStyle | WS_EX_TOOLWINDOW;
    }
    m_dwStyle = child ? UI_WNDSTYLE_CHILD : m_dwStyle;
    CDuiString strWndName = GetWndName();
    CWindowWnd::Create(hParentWnd, strWndName, m_dwStyle, m_dwExStyle, nPosX, nPosY, 0, 0, 0);
    return m_hWnd;
}

LRESULT WndBase::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LONG styleValue = ::GetWindowLong(m_hWnd, GWL_STYLE);
    styleValue &= ~WS_CAPTION;
    ::SetWindowLong(m_hWnd, GWL_STYLE, styleValue);
    SetIcon(IDI_VIDEOLOADER);
    m_pm.Init(m_hWnd);
    CDialogBuilder builder;
    CDuiString strXmlPath = GetXmlPath();
    CControlUI* pRoot = builder.Create(strXmlPath.GetData(), NULL, this, &m_pm);
    if (!pRoot) {
        CDuiString strInfo;
        strInfo.Format(L"\n%sxml解析失败", strXmlPath);
        OutputDebugString(strInfo.GetData());
        return -1;
    }
    m_pm.AttachDialog(pRoot);
    m_pm.AddNotifier(this);
    InitWindow();
    return 0;
}

LRESULT WndBase::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == SC_CLOSE && QuitOnSysClose())
        return 0;
    BOOL bZoomed = ::IsZoomed(m_hWnd);
    LRESULT lRes = CWindowWnd::HandleMessage(uMsg, wParam, lParam);
    if (wParam == SC_CLOSE)
        return 0;
    if (::IsZoomed(m_hWnd) != bZoomed)
    {
        CControlUI* pCtrl = m_pm.FindControl(L"btn_sys_max");
        if (pCtrl)
            pCtrl->SetVisible(TRUE == bZoomed);
        pCtrl = m_pm.FindControl(L"btn_sys_restore");
        if (pCtrl)
            pCtrl->SetVisible(FALSE == bZoomed);
    }
    return lRes;
}

LRESULT WndBase::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
    ::ScreenToClient(m_hWnd, &pt);

    RECT rcClient;
    ::GetClientRect(m_hWnd, &rcClient);

    if (!::IsZoomed(m_hWnd)) {
        RECT rcSizeBox = m_pm.GetSizeBox();
        if (pt.y < rcClient.top + rcSizeBox.top) {
            if (pt.x < rcClient.left + rcSizeBox.left) return HTTOPLEFT;
            if (pt.x > rcClient.right - rcSizeBox.right) return HTTOPRIGHT;
            return HTTOP;
        }
        else if (pt.y > rcClient.bottom - rcSizeBox.bottom) {
            if (pt.x < rcClient.left + rcSizeBox.left) return HTBOTTOMLEFT;
            if (pt.x > rcClient.right - rcSizeBox.right) return HTBOTTOMRIGHT;
            return HTBOTTOM;
        }
        if (pt.x < rcClient.left + rcSizeBox.left) return HTLEFT;
        if (pt.x > rcClient.right - rcSizeBox.right) return HTRIGHT;
    }

    RECT rcCaption = m_pm.GetCaptionRect();
    if (pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right \
        && pt.y >= rcCaption.top && pt.y < rcCaption.bottom) {
        CControlUI* pControl = static_cast<CControlUI*>(m_pm.FindControl(pt));
        if (pControl && _tcscmp(pControl->GetClass(), _T("ButtonUI")) != 0 &&
            _tcscmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
            _tcscmp(pControl->GetClass(), _T("EditUI")) != 0 &&
            _tcscmp(pControl->GetClass(), _T("ButtonGifUI")) != 0)
            return HTCAPTION;
    }

    return HTCLIENT;
}

LRESULT WndBase::OnNcActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (::IsIconic(m_hWnd)) bHandled = FALSE;
    return (wParam == 0) ? TRUE : FALSE;
}

LRESULT WndBase::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    MONITORINFO oMonitor = {};
    oMonitor.cbSize = sizeof(oMonitor);
    ::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &oMonitor);
    CDuiRect rcWork = oMonitor.rcWork;
    rcWork.Offset(-rcWork.left, -rcWork.top);

    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    lpMMI->ptMaxPosition.x = rcWork.left;
    lpMMI->ptMaxPosition.y = rcWork.top;
    lpMMI->ptMaxSize.x = rcWork.right;
    lpMMI->ptMaxSize.y = rcWork.bottom;

    bHandled = FALSE;
    return 0;
}

LRESULT WndBase::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    BOOL bHandled = TRUE;
    switch (uMsg)
    {
    case WM_CREATE:
        lResult = OnCreate(uMsg, wParam, lParam, bHandled);
        break;
    case WM_SYSCOMMAND: lResult = OnSysCommand(uMsg, wParam, lParam, bHandled); break;
    case WM_NCHITTEST: lResult = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
    case WM_NCACTIVATE: lResult = OnNcActivate(uMsg, wParam, lParam, bHandled); break;
    case WM_GETMINMAXINFO: lResult = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
    case WM_NCCALCSIZE: lResult = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
    case WM_NCPAINT: lResult = OnNcPaint(uMsg, wParam, lParam, bHandled); break;
    case WM_SIZE: lResult = OnSize(uMsg, wParam, lParam, bHandled); break;
    case WM_TIMER: lResult = OnTimer(wParam, lParam); bHandled = FALSE; break;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE && m_bEscape)
        {
            if (OnEscape())
                Close(IDCLOSE);
        }
        bHandled = FALSE;
        break;
    default: bHandled = FALSE; break;
    }
    if (bHandled) return lResult;
    if (m_pm.MessageHandler(uMsg, wParam, lParam, lResult))
        return lResult;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

LRESULT WndBase::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SIZE szRoundCorner = m_pm.GetRoundCorner();
    if (!::IsIconic(m_hWnd))
    {
        if (szRoundCorner.cx != 0 || szRoundCorner.cy != 0)
        {
            CDuiRect rcWnd;
            ::GetWindowRect(m_hWnd, &rcWnd);
            rcWnd.Offset(-rcWnd.left, -rcWnd.top);
            rcWnd.right++; rcWnd.bottom++;
            HRGN hRgn = ::CreateRoundRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, szRoundCorner.cx, szRoundCorner.cy);
            ::SetWindowRgn(m_hWnd, hRgn, TRUE);
            ::DeleteObject(hRgn);

        }
    }
    bHandled = FALSE;
    return 0;
}

LRESULT WndBase::OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT WndBase::OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT WndBase::OnTimer(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

void WndBase::InitWindow()
{
    InitControls();
    BindControls();
}

CControlUI* WndBase::CreateControl(LPCTSTR pstrClass)
{
    return NULL;
}

bool WndBase::QuitOnSysClose()
{
    return false;
}

void WndBase::SetWindowStyle(DWORD dwStyle)
{
    m_dwStyle = dwStyle;
}

void WndBase::SetWindowExStyle(DWORD dwExStyle)
{
    m_dwExStyle = dwExStyle;
}

void WndBase::Notify(TNotifyUI& msg)
{
    return CNotifyPump::NotifyPump(msg);
}

bool WndBase::OnEscape()
{
    return true;
}

void WndBase::OnNotifyClick(TNotifyUI& msg)
{
    OnClick(msg);
}
