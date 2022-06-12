#pragma once
#include "wnd_base.h"
#include "async/async_service_delegate.h"

class WndInfo
    : public WndBase
    , public IAsyncServiceDelegate
{
    // Ã¶¾ÙÒ³Ãæ
    enum {
        PAGE_START = 0,
        PAGE_LOADING,
        PAGE_SUCCESS,
    };
public:
    WndInfo();
    ~WndInfo();
    void SetUrl(std_cwstr_ref url);

protected:
    LPCWSTR GetWndName()const override;
    LPCWSTR GetXmlPath()const override {
        return L"wnd_info.xml";
    }
    void InitWindow() override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    BEGIN_INIT_CTRL
        DECLARE_CTRL_TYPE(url_edit_, CEditUI, L"edit")
        DECLARE_CTRL_TYPE(main_tab_, CTabLayoutUI, L"main")
        DECLARE_CTRL_TYPE(combobox_, CComboUI, L"combo_qn")
        DECLARE_CTRL(title_, L"label_title")
        DECLARE_CTRL(date_, L"label_date")
        DECLARE_CTRL(author_, L"label_author")
        DECLARE_CTRL(duration_, L"label_duration")
        DECLARE_CTRL(cover_, L"ctrl_cover")
        DECLARE_CTRL_BIND(btn_download_, CControlUI, L"btn_download", &WndInfo::OnNotifyDownload)
    END_INIT_CTRL

    BEGIN_BIND_CTRL
        BIND_CTRL(L"btn_close", &WndInfo::OnNotifyClose)
        BIND_CTRL(L"btn_parse", &WndInfo::OnNotifyParse)
    END_BIND_CTRL
    bool OnNotifyClose(void* param);
    bool OnNotifyParse(void* param);
    bool OnNotifyDownload(void* param);

    bool OnClickPraseUrl(const CDuiString& text);
    bool OnClickDownload();

    void OnAsyncComplete(AsyncTaskType task_type, AsyncErrorCode code, void* data, void* param) override;
    LRESULT OnMsgAsyncSuccess(WPARAM wParam, LPARAM lParam);
    LRESULT OnMsgAsyncError(WPARAM wParam, LPARAM lParam);
    void OnTaskGetInfo(LPARAM lParam);
    void OnTaskGetPlayerUrl(LPARAM lParam);
    void OnTaskGetSelectPlayerUrl(LPARAM lParam);
    void OnTaskDownloadImage(LPARAM lParam);

private:
    CEditUI* url_edit_ = nullptr;
    CTabLayoutUI* main_tab_ = nullptr;
    CControlUI* title_ = nullptr;
    CControlUI* author_ = nullptr;
    CControlUI* date_ = nullptr;
    CControlUI* duration_ = nullptr;
    CControlUI* cover_ = nullptr;
    CControlUI* btn_download_ = nullptr;
    CComboUI* combobox_ = nullptr;
    std::unique_ptr<VideoInfo> video_info_;
    std::unique_ptr< PlayerInfo> player_info_;
    std::wstring image_path_;
    VideoType video_type_ = VideoType::VIDEO_UNKNOWN;
};

