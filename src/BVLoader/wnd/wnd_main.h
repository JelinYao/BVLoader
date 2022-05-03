#pragma once
#include "wnd_base.h"
#include "download/download_service_delegate.h"
#include <unordered_map>
#include "message_define.h"
#include "audio/audio_service_delegate.h"

namespace download {
    class Task;
}

class WndMain
    : public WndBase
    , public download::IDownloadServiceDelegate
    , public IAudioServiceDelegate
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

    BEGIN_INIT_CTRL
        DECLARE_CTRL_TYPE(tab_main_, CTabLayoutUI, L"tab_main")
        DECLARE_CTRL_TYPE(opt_selall_loading, COptionUI, L"checkbox_ing")
        DECLARE_CTRL_TYPE(opt_selall_finish, COptionUI, L"checkbox_done")
        DECLARE_CTRL_TYPE(list_loading_, CListUI, L"list_loading")
        DECLARE_CTRL_TYPE(list_finish_, CListUI, L"list_finish")
    END_INIT_CTRL
    BEGIN_BIND_CTRL
        BIND_CTRL(L"btn_close", &WndMain::OnNotifyClose)
        BIND_CTRL(L"btn_download", &WndMain::OnNotifyDownload)
    END_BIND_CTRL
    bool OnNotifyClose(void* param);
    bool OnNotifyDownload(void* param);
    void Notify(TNotifyUI& msg) override;
    void OnClick(TNotifyUI& msg) override;

    bool AddLoadingItem(const shared_ptr<download::Task>& task);
    bool OnNotifySelectLoadingListItem(void* param);
    void OnClickStartLoading(CControlUI* sender);
    void OnClickStopLoading(CControlUI* sender);
    void OnClickDeleteLoading(CControlUI* sender);
    bool OnNotifyListItem(void* param);

    UINT ShowMsgBox(MessageIconType icon, LPCWSTR info);
    LRESULT OnMsgMsgbox(WPARAM wParam, LPARAM lParam);
    LRESULT OnWparamDeleteLoadingItem(LPARAM lParam);
    LRESULT OnMsgDecode(WPARAM wParam, LPARAM lParam);

private:
    bool need_exit_ = false;
    HWND hwnd_parse_ = NULL;
    std::unique_ptr<WndBase> wnd_parse_;
    CTabLayoutUI* tab_main_ = nullptr;
    COptionUI* opt_selall_loading = nullptr;
    COptionUI* opt_selall_finish = nullptr;
    CListUI* list_loading_ = nullptr;
    CListUI* list_finish_ = nullptr;
    std::unordered_map<UINT_PTR, CContainerUI*> map_loading_items_;
};

