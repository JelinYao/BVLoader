#pragma once
#include "wnd_base.h"
#include "download/download_service_delegate.h"
#include <unordered_map>
#include "message_define.h"
#include "audio/audio_service_delegate.h"
#include "async/async_service_delegate.h"

namespace download {
    class Task;
}

class WndMain
    : public WndBase
    , public download::IDownloadServiceDelegate
    , public IAudioServiceDelegate
    , public IAsyncServiceDelegate
{
public:
    WndMain();
    ~WndMain();

protected:
    LPCWSTR GetWndName()const override;
    LPCWSTR GetXmlPath()const override {
        return L"wnd_main.xml";
    }

    void InitWindow() override;
    void OnFinalMessage(HWND hWnd) override;
    bool QuitOnSysClose() override;
    void Close(UINT nRet = IDOK) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    // download service callback
    void NotifyStatus(const std::shared_ptr<download::Task>& task, void* param) override;
    void NotifyProgress(const std::shared_ptr<download::Task>& task, void* param, int speed) override;
    // audio service callback
    void OnDecodeComplete(UINT_PTR task_id, DecodeErrorCode code, void* data) override;
    // async service callback
    void OnAsyncComplete(AsyncTaskType task_type, AsyncErrorCode code, void* data, void* param) override;

    BEGIN_INIT_CTRL
        DECLARE_CTRL_TYPE(tab_main_, CTabLayoutUI, L"tab_main")
        DECLARE_CTRL_TYPE(opt_selall_loading, COptionUI, L"checkbox_ing")
        DECLARE_CTRL_TYPE(opt_selall_finish, COptionUI, L"checkbox_done")
        DECLARE_CTRL_TYPE(list_loading_, CListUI, L"list_loading")
        DECLARE_CTRL_TYPE(list_finish_, CListUI, L"list_finish")
        DECLARE_CTRL(btn_user_, L"btn_user")
    END_INIT_CTRL
    BEGIN_BIND_CTRL
    END_BIND_CTRL

    void Notify(TNotifyUI& msg) override;
    void OnClick(TNotifyUI& msg) override;

    bool AddLoadingItem(const shared_ptr<download::Task>& task);
    bool AddFinishItem(const shared_ptr<download::Task>& task);

    bool OnNotifySelectLoadingListItem(void* param);
    void OnClickStartLoading(CControlUI* sender);
    void OnClickStopLoading(CControlUI* sender);
    void OnClickDeleteLoading(CControlUI* sender);
    void OnClickFinishLoading(CControlUI* sender);
    bool OnNotifyListItem(void* param);
    void ShowLogin();
    void ShowDownload();

    UINT ShowMsgBox(MessageIconType icon, LPCWSTR info);
    LRESULT OnMsgMsgbox(WPARAM wParam, LPARAM lParam);
    LRESULT OnWparamDeleteItem(LPARAM lParam);
    LRESULT OnMsgDecode(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgNotifyStatus(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgShowWnd(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgLoginSuccess(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgAsyncSuccess(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgAsyncError(WPARAM wParam, LPARAM lParam);

private:
    bool user_login_ = false;
    bool need_exit_ = false;
    HWND hwnd_parse_ = NULL;
    HWND hwnd_login_ = NULL;
    std::unique_ptr<WndBase> wnd_parse_;
    std::unique_ptr<WndBase> wnd_login_;
    CTabLayoutUI* tab_main_ = nullptr;
    COptionUI* opt_selall_loading = nullptr;
    COptionUI* opt_selall_finish = nullptr;
    CListUI* list_loading_ = nullptr;
    CListUI* list_finish_ = nullptr;
    CControlUI* btn_user_ = nullptr;
    std::unordered_map<UINT_PTR, CContainerUI*> map_loading_items_;
};

