#include "pch.h"
#include "wnd_login.h"
#include "soft_define.h"
#include "service_manager.h"
#include "message_define.h"
#include "qrcode_view.h"
#include "api_define.h"

WndLogin::WndLogin(HWND main_wnd)
    : main_wnd_(main_wnd)
{
    m_dwStyle = UI_WNDSTYLE_FRAME ^ WS_MAXIMIZEBOX;
    m_bEscape = true;
    m_bDeleteThis = false;
}

WndLogin::~WndLogin()
{
}

LPCWSTR WndLogin::GetWndName() const
{
    return kWndLoginTitle;
}

void WndLogin::InitWindow()
{
    WndBase::InitWindow();
    ASYNC_SERVICE()->AddDelegate(this, nullptr);
    RefreshQrcode();
}

void WndLogin::OnFinalMessage(HWND hWnd)
{
    ASYNC_SERVICE()->RemoveDelegate(this);
    WndBase::OnFinalMessage(hWnd);
}

void WndLogin::OnClick(TNotifyUI& msg)
{
    if (msg.pSender->GetName() == L"btn_close") {
        Close();
    }
}

CControlUI* WndLogin::CreateControl(LPCTSTR pstrClass)
{
    if (wcscmp(pstrClass, L"QrcodeView") == 0) {
        qrcode_view_ = new QrcodeView(&m_pm);
        qrcode_view_->OnNotify += MakeDelegate(this, &WndLogin::OnNotifyQrcodeView);
        return qrcode_view_;
    }
    return nullptr;
}

LRESULT WndLogin::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_QRCODEWND_ASYNC_SUCCESS:
        return OnMsgAsyncSuccess(wParam, lParam);
    case WM_QRCODEWND_ASYNC_ERROR:
        return OnMsgAsyncError(wParam, lParam);
    default:
        break;
    }
    return WndBase::HandleMessage(uMsg, wParam, lParam);
}

LRESULT WndLogin::OnTimer(WPARAM wParam, LPARAM lParam)
{
    if (wParam == TIMER_ID_QRCODEWND_LOGIN) {
        StopLoginTimer();
        ASYNC_SERVICE()->AddLoginTask(kRequestLoginInfo, auth_key_);
    }
    return 0;
}

bool WndLogin::OnNotifyQrcodeView(void* param)
{
    TNotifyUI* notify = reinterpret_cast<TNotifyUI*>(param);
    if (notify->sType == kQrcodeViewClickRefreshMessage) {
        RefreshQrcode();
    }
    return true;
}

void WndLogin::StartLoginTimer()
{
    ::SetTimer(m_hWnd, TIMER_ID_QRCODEWND_LOGIN, kQrcodeLoginTimerElapse, NULL);
}

void WndLogin::StopLoginTimer()
{
    ::KillTimer(m_hWnd, TIMER_ID_QRCODEWND_LOGIN);
}

void WndLogin::RefreshQrcode()
{
    StopLoginTimer();
    qrcode_view_->ShowLoading();
    ASYNC_SERVICE()->AddHttpTask(AsyncTaskType::TASK_GET_LOGIN_URL, kRequestLoginUrl);
}

void WndLogin::OnAsyncComplete(AsyncTaskType task_type, AsyncErrorCode code, void* data, void* param)
{
    if (task_type == AsyncTaskType::TASK_GET_LOGIN_URL
        || task_type == AsyncTaskType::TASK_GET_LOGIN_INFO) {
        // ��UI�߳�ת��
        UINT msg = (code == AsyncErrorCode::ERROR_SUCCESS) ? WM_QRCODEWND_ASYNC_SUCCESS : WM_QRCODEWND_ASYNC_ERROR;
        ::PostMessage(m_hWnd, msg, (WPARAM)task_type, (LPARAM)data);
    }
}

LRESULT WndLogin::OnMsgAsyncSuccess(WPARAM wParam, LPARAM lParam)
{
    AsyncTaskType task_type = static_cast<AsyncTaskType>(wParam);
    switch (task_type)
    {
    case AsyncTaskType::TASK_GET_LOGIN_URL: {
        auto info = reinterpret_cast<QrcodeUrlInfo*>(lParam);
        assert(info);
        std::unique_ptr<QrcodeUrlInfo> auto_ptr(info);
        qrcode_view_->SetQrcodeUrl(std::move(info->url));
        auth_key_ = std::move(info->auth_key);
        // ������ʱ��
        StartLoginTimer();
        break;
    }
    case AsyncTaskType::TASK_GET_LOGIN_INFO: {
        auto info = reinterpret_cast<QrcodeLoginInfo*>(lParam);
        assert(info);
        std::unique_ptr<QrcodeLoginInfo> auto_ptr(info);
        switch (info->code)
        {
        case 0: {
            // ��½�ɹ�
            ::PostMessage(main_wnd_, WM_MAINWND_LOGIN_SUCCESS, 0, 0);
            Close();
            break;
        }
        case -1: // oauthKey is wrong. should never be this case            break;
        case -2: // login url (qrcode) is expired
            // ��ά�����
            qrcode_view_->ShowExpire(kTextQrcodeExpired);
            break;
        case -4: // qrcode not scanned
            StartLoginTimer();
            break;
        case -5: // scanned but not confirmed
            qrcode_view_->ShowExpire(kTextQrcodeConfirmed);
            StartLoginTimer();
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

LRESULT WndLogin::OnMsgAsyncError(WPARAM wParam, LPARAM lParam)
{
    AsyncTaskType task_type = static_cast<AsyncTaskType>(wParam);
    switch (task_type)
    {
    case AsyncTaskType::TASK_GET_LOGIN_URL: {
        break;
    }
    case AsyncTaskType::TASK_GET_LOGIN_INFO: {
        qrcode_view_->ShowExpire(kTextQrcodeRequestFailed);
        break;
    }
    default:
        break;
    }
    return 0;
}
