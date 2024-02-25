#pragma once
#include "wnd_base.h"
#include "async/async_service_delegate.h"

class QrcodeView;
class WndLogin
    : public WndBase
    , public IAsyncServiceDelegate
{
public:
    WndLogin(HWND main_wnd);
    ~WndLogin();

protected:
    LPCWSTR GetWndName()const override;
    LPCWSTR GetXmlPath()const override { return L"wnd_login.xml"; }
    void InitWindow() override;
    void OnFinalMessage(HWND hWnd) override;
    void OnClick(TNotifyUI& msg) override;
    CControlUI* CreateControl(LPCTSTR pstrClass) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    LRESULT OnTimer(WPARAM wParam, LPARAM lParam) override;
    bool OnNotifyQrcodeView(void* param);
    void StartLoginTimer();
    void StopLoginTimer();

    BEGIN_INIT_CTRL
    END_INIT_CTRL
    BEGIN_BIND_CTRL
    END_BIND_CTRL

    void RefreshQrcode();
    void OnAsyncComplete(AsyncTaskType task_type, AsyncErrorCode code, void* data, void* param) override;
    LRESULT OnMsgAsyncSuccess(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgAsyncError(WPARAM wParam, LPARAM lParam);

    LRESULT OnMsgGetQrcode(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgGetLoginResult(WPARAM wParam, LPARAM lParam);

private:
    QrcodeView* qrcode_view_ = nullptr;
    std_str auth_key_;
    HWND main_wnd_ = NULL;
};