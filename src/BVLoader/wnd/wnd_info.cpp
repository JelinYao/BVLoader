#include "pch.h"
#include "wnd_info.h"
#include "url_parse.h"
#include "service_manager.h"
#include "message_define.h"
#include "soft_define.h"

WndInfo::WndInfo()
{
    m_bDeleteThis = false;
    m_dwStyle = UI_WNDSTYLE_FRAME ^ WS_MAXIMIZEBOX;
}

WndInfo::~WndInfo()
{
}

LPCWSTR WndInfo::GetWndName() const
{
    return kWndParseTitle;
}

void WndInfo::InitWindow()
{
    __super::InitWindow();
    auto service = ServiceManager::Instance()->GetAsyncService();
    assert(service != nullptr);
    service->AddDelegate(this, nullptr);
}

LRESULT WndInfo::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PARSEWND_ASYNC_SUCCESS:
        return OnMsgAsyncSuccess(wParam, lParam);
    case WM_PARSEWND_ASYNC_ERROR:
        return OnMsgAsyncError(wParam, lParam);
    default:
        break;
    }
    return __super::HandleMessage(uMsg, wParam, lParam);
}

bool WndInfo::OnNotifyClose(void* param)
{
    TNotifyUI* notify = reinterpret_cast<TNotifyUI*>(param);
    if (notify->sType == DUI_MSGTYPE_CLICK) {
        ShowWindow(false, false);
        main_tab_->SelectItem(PAGE_START);
        image_path_.clear();
        url_edit_->SetText(L"");
    }
    return true;
}

bool WndInfo::OnNotifyParse(void* param)
{
    TNotifyUI* notify = reinterpret_cast<TNotifyUI*>(param);
    if (notify->sType == DUI_MSGTYPE_CLICK) {
        auto text = url_edit_->GetText();
        if (OnClickPraseUrl(text)) {
            main_tab_->SelectItem(PAGE_LOADING);
        }
    }
    return true;
}

bool WndInfo::OnNotifyDownload(void* param)
{
    TNotifyUI* notify = reinterpret_cast<TNotifyUI*>(param);
    if (notify->sType == DUI_MSGTYPE_CLICK) {
        OnClickDownload();
    }
    return true;
}

bool WndInfo::OnClickPraseUrl(const CDuiString& text)
{
    if (text.IsEmpty()) {
        
        return false;
    }
    auto url = string_utils::UToUtf8(text.GetData());
    std_str detailUrll;
    if (!ParseBiliUrl(url, detailUrll, video_type_)) {
        LOG(ERROR) << "OnClickPraseUrl 解析视频信息失败";
        return false;
    }
    ASYNC_SERVICE()->AddVideoTask(AsyncTaskType::TASK_GET_INFO, video_type_, detailUrll);
    return true;
}

bool WndInfo::OnClickDownload()
{
    // 获取视频下载地址
    if (!video_info_ || !player_info_) {
        LOG(ERROR) << "OnClickDownload 获取视频信息失败";
        return false;
    }
    int index = combobox_->GetCurSel();
    if (index < 0 || index >= (int)player_info_->qn.size()) {
        LOG(ERROR) << "OnClickDownload 数组越界";
        return false;
    }
    // 构造请求地址
    auto url = BuildPlayerUrl(video_type_, video_info_->aid, video_info_->cid, player_info_->qn[index].value);
    ASYNC_SERVICE()->AddHttpTask(AsyncTaskType::TASK_GET_SELECT_PLAYER_URL, url);
    btn_download_->SetEnabled(false);
    return true;
}

void WndInfo::OnAsyncComplete(AsyncTaskType task_type, AsyncErrorCode code, void* data, void* param)
{
    // 往UI线程转发
    if (code == AsyncErrorCode::ERROR_SUCCESS) {
        ::PostMessage(m_hWnd, WM_PARSEWND_ASYNC_SUCCESS, (WPARAM)task_type, (LPARAM)data);
    }
    else {
        ::PostMessage(m_hWnd, WM_PARSEWND_ASYNC_ERROR, (WPARAM)task_type, (LPARAM)data);
    }
    
}

LRESULT WndInfo::OnMsgAsyncSuccess(WPARAM wParam, LPARAM lParam)
{
    AsyncTaskType type = static_cast<AsyncTaskType>(wParam);
    switch (type)
    {
    case AsyncTaskType::TASK_NONE:
        break;
    case AsyncTaskType::TASK_GET_INFO:
        OnTaskGetInfo(lParam);
        break;
    case AsyncTaskType::TASK_GET_PLAYER_URL:
        OnTaskGetPlayerUrl(lParam);
        break;
    case AsyncTaskType::TASK_GET_SELECT_PLAYER_URL:
        OnTaskGetSelectPlayerUrl(lParam);
        break;
    case AsyncTaskType::TASK_DOWNLOAD_IMAGE:
        OnTaskDownloadImage(lParam);
        break;
    default:
        break;
    }
    return 0;
}

LRESULT WndInfo::OnMsgAsyncError(WPARAM wParam, LPARAM lParam)
{
    AsyncTaskType type = static_cast<AsyncTaskType>(wParam);
    switch (type)
    {
    case AsyncTaskType::TASK_NONE:
        break;
    case AsyncTaskType::TASK_GET_INFO:
        // 获取视频信息失败
        main_tab_->SelectItem(PAGE_START);
        break;
    case AsyncTaskType::TASK_GET_PLAYER_URL:
        break;
    case AsyncTaskType::TASK_GET_SELECT_PLAYER_URL:
        break;
    case AsyncTaskType::TASK_DOWNLOAD_IMAGE:
        image_path_.clear();
        break;
    default:
        break;
    }
    return 0;
}

void WndInfo::OnTaskGetInfo(LPARAM lParam)
{
    VideoInfo* info = reinterpret_cast<VideoInfo*>(lParam);
    assert(info);
    std::unique_ptr<VideoInfo> auto_ptr(info);
    video_info_.swap(auto_ptr);
    title_->SetText(info->title.c_str());
    CDuiString text;
    text.Format(L"作者：%s", info->author.c_str());
    author_->SetText(text);
    main_tab_->SelectItem(PAGE_SUCCESS);
    text.Format(L"发布日期：%s", string_utils::TimeStampToString(info->ctime, false).c_str());
    date_->SetText(text);
    text.Format(L"时长：%s", string_utils::SecondsToString(info->duration).c_str());
    duration_->SetText(text);
    btn_download_->SetEnabled(false);
}

void WndInfo::OnTaskGetPlayerUrl(LPARAM lParam)
{
    PlayerInfo* info = reinterpret_cast<PlayerInfo*>(lParam);
    assert(info);
    std::unique_ptr<PlayerInfo> auto_ptr(info);
    player_info_.swap(auto_ptr);
    for (auto& qn : info->qn) {
        CListLabelElementUI* elem = new CListLabelElementUI;
        elem->SetText(string_utils::Utf8ToU(qn.desc).c_str());
        combobox_->Add(elem);
    }
    combobox_->SelectItem(0);
    btn_download_->SetEnabled(true);
}

void WndInfo::OnTaskGetSelectPlayerUrl(LPARAM lParam)
{
    PlayerInfo* info = reinterpret_cast<PlayerInfo*>(lParam);
    assert(info);
    std::unique_ptr<PlayerInfo> auto_ptr(info);
    // 拿到了下载地址
    main_tab_->SelectItem(PAGE_START);
    combobox_->RemoveAll();
    auto service = DOWNLOAD_SERVICE();
    service->AddTask(info->url, video_info_->title, image_path_, video_info_->author, video_info_->duration, video_info_->ctime);
    url_edit_->SetText(L"");
    cover_->SetBkImage(kDefaultCover);
    ShowWindow(false, false);
}

void WndInfo::OnTaskDownloadImage(LPARAM lParam)
{
    ImageInfo* info = reinterpret_cast<ImageInfo*>(lParam);
    assert(info);
    if (info->image_type == ImageType::IMAGE_VIDEO_COVER) {
        std::unique_ptr<ImageInfo> auto_ptr(info);
        if (PathFileExists(auto_ptr->save_path.c_str())) {
            cover_->SetBkImage(auto_ptr->save_path.c_str());
        }
        image_path_ = std::move(auto_ptr->save_path);
    }
}
