#include "pch.h"
#include "wnd_main.h"
#include "soft_define.h"
#include "wnd_info.h"
#include "service_manager.h"
#include "download/task.h"
#include "wnd_msg.h"
#include "wnd_login.h"

using namespace download;
WndMain::WndMain()
{
    m_bDeleteThis = false;
    m_dwStyle = UI_WNDSTYLE_FRAME ^ WS_MAXIMIZEBOX;
}

WndMain::~WndMain()
{
}

LPCWSTR WndMain::GetWndName() const
{
    return kAppWindowTitle;
}

void WndMain::InitWindow()
{
    __super::InitWindow();
    wnd_parse_ = std::make_unique<WndInfo>();
    hwnd_parse_ = wnd_parse_->Create(m_hWnd);
    wnd_parse_->ShowWindow(false, false);
    DOWNLOAD_SERVICE()->AddDelegate(this, nullptr);
    AUDIO_SERVICE()->AddDelegate(this, nullptr);
}

void WndMain::OnFinalMessage(HWND hWnd)
{
    if (::IsWindow(hwnd_parse_)) {
        ::SendMessage(hwnd_parse_, WM_CLOSE, 0, 0);
    }
    __super::OnFinalMessage(hWnd);
}

bool WndMain::QuitOnSysClose()
{
    Close();
    return false;
}

void WndMain::Close(UINT nRet /*= IDOK*/)
{
    need_exit_ = true;
    __super::Close(nRet);
    ServiceManager::Instance()->Exit();
    ::PostQuitMessage(0);
}

LRESULT WndMain::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MAINWND_MSGBOX:
        return OnMsgMsgbox(wParam, lParam);
    case WM_MAINWND_DECODE:
        return OnMsgDecode(wParam, lParam);
    case WM_MAINWND_NOTIFY_STATUS:
        return OnMsgNotifyStatus(wParam, lParam);
    default:
        break;
    }
    return __super::HandleMessage(uMsg, wParam, lParam);
}

void WndMain::NotifyStatus(const std::shared_ptr<download::Task>& task, void* param)
{
    if (task->status == download::DownloadStatus::STATUS_INIT) {
        AddLoadingItem(task);
        return;
    }
    UINT_PTR task_id = (UINT_PTR)task.get();
    auto iter = map_loading_items_.find(task_id);
    if (iter == map_loading_items_.end()) {
        LOG(ERROR) << "NotifyStatus 未找到下载节点";
        return;
    }
    auto label_state = iter->second->FindSubControl(L"lbl_state");
    assert(label_state);
    auto btn_start = iter->second->FindSubControl(L"btn_loading_start");
    assert(btn_start);
    auto btn_stop = iter->second->FindSubControl(L"btn_loading_stop");
    assert(btn_stop);
    auto btn_size = iter->second->FindSubControl(L"lbl_size");
    assert(btn_size);
    switch (task->status)
    {
    case download::DownloadStatus::STATUS_LOADING: {
        label_state->SetText(kTextLoading);
        btn_start->SetVisible(false);
        btn_stop->SetVisible();
        btn_size->SetVisible();
        break;
    }
    case download::DownloadStatus::STATUS_PAUSE: {
        label_state->SetText(kTextLoadPause);
        btn_start->SetVisible();
        btn_stop->SetVisible(false);
        btn_size->SetVisible(false);
        break;
    }
    case download::DownloadStatus::STATUS_WAITTING: {
        label_state->SetText(kTextLoadWaiting);
        btn_start->SetVisible(false);
        btn_stop->SetVisible();
        btn_size->SetVisible(false);
        break;
    }
    case download::DownloadStatus::STATUS_FAILED: {
        label_state->SetText(kTextLoadFailed);
        auto progress = dynamic_cast<CProgressUI*>(iter->second->FindSubControl(L"progress"));
        assert(progress);
        progress->SetForeImage(L"pro_error.png");
        btn_start->SetVisible(false);
        btn_stop->SetVisible();
        btn_size->SetVisible(false);
        break;
    }
    case download::DownloadStatus::STATUS_DOWNLOAD_SUCCESS: {
        label_state->SetText(kTextLoadFinish);
        auto progress = dynamic_cast<CProgressUI*>(iter->second->FindSubControl(L"progress"));
        assert(progress);
        progress->SetValue(100);
        btn_size->SetVisible();
        btn_size->SetText(string_utils::SecondsToString(task->duration).c_str());
        // 解码
        task->status = download::DownloadStatus::STATUS_DECODE;
        ASYNC_SERVICE()->AddDecodeTask(task_id, task->save_path, task->audio_path);
        ::PostMessage(m_hWnd, WM_MAINWND_NOTIFY_STATUS, (WPARAM)task_id, (LPARAM)param);
        break;
    }
    case download::DownloadStatus::STATUS_DECODE: {
        label_state->SetText(kTextDecoding);
        btn_stop->SetVisible(false);
        auto btn_delete = iter->second->FindSubControl(L"btn_loading_delete");
        assert(btn_delete);
        btn_delete->SetVisible(false);
        break;
    }
    case download::DownloadStatus::STATUS_FINISH: {
        // 完成后，加入完成列表
        list_loading_->Remove(iter->second);
        map_loading_items_.erase(iter);
        auto task = DOWNLOAD_SERVICE()->FindLoadingTask(task_id);
        if (task == nullptr) {
            break;
        }
        if (DOWNLOAD_SERVICE()->AddFinishTask(task_id)) {
            AddFinishItem(task);
        }
        break;
    }
    default:
        break;
    }
}

void WndMain::NotifyProgress(const std::shared_ptr<download::Task>& task, void* param, int speed)
{
    UINT_PTR task_ptr = (UINT_PTR)task.get();
    auto iter = map_loading_items_.find(task_ptr);
    if (iter == map_loading_items_.end()) {
        LOG(ERROR) << "NotifyStatus 未找到下载对象";
        return;
    }
    auto progress = dynamic_cast<CProgressUI*>(iter->second->FindSubControl(L"progress"));
    assert(progress);
    int percent = 0;
    if (task->total_size > 0.0F) {
        percent = (int)((task->load_size * 100) / task->total_size);
    }
    progress->SetValue(percent);
    auto ctrl = iter->second->FindSubControl(L"lbl_size");
    if (ctrl) {
        CDuiString text;
        if (speed > 1024) {
            text.Format(L"%0.2f MB/s", speed / 1024.0);
        }
        else {
            text.Format(L"%d KB/s", speed);
        }
        ctrl->SetText(text);
    }
}

void WndMain::OnDecodeComplete(UINT_PTR task_id, DecodeErrorCode code, void* data)
{
    if (need_exit_) {
        return;
    }
    ::PostMessage(m_hWnd, WM_MAINWND_DECODE, (WPARAM)task_id, (LPARAM)code);
}

void WndMain::Notify(TNotifyUI& msg)
{
    if (msg.sType == DUI_MSGTYPE_SELECTCHANGED) {
        if (msg.pSender->GetName().Compare(L"opt_loading") == 0) {
            tab_main_->SelectItem(0);
        }
        else if (msg.pSender->GetName().Compare(L"opt_finish") == 0) {
            tab_main_->SelectItem(1);
        }
        else if (msg.pSender->GetName().Compare(L"opt_select_loading") == 0) {
            
        }
    }
    WndBase::Notify(msg);
}

void WndMain::OnClick(TNotifyUI& msg)
{
    auto& name = msg.pSender->GetName();
    if (wcsncmp(name.GetData(), L"btn_loading", 11) == 0) {
        if (name.Compare(L"btn_loading_start") == 0) {
            OnClickStartLoading(msg.pSender);
        }
        else if (name.Compare(L"btn_loading_stop") == 0) {
            OnClickStopLoading(msg.pSender);
        }
        else if (name.Compare(L"btn_loading_delete") == 0) {
            OnClickDeleteLoading(msg.pSender);
        }
        return;
    }
    if (name.Compare(L"btn_finish_delete") == 0) {
        OnClickFinishLoading(msg.pSender);
    }
    else if (name.Compare(L"btn_close") == 0) {
        Close();
    }
    else if (name.Compare(L"btn_download") == 0) {
        ShowLogin();
        assert(wnd_parse_);
        wnd_parse_->CenterWindow();
        wnd_parse_->ShowWindow();
    }
}

bool WndMain::AddLoadingItem(const shared_ptr<download::Task>& task)
{
    CDialogBuilder builder;
    auto item = dynamic_cast<CListContainerElementUI*>(builder.Create(L"item_loading.xml", (LPCTSTR)0, this, &m_pm));
    assert(item);
    UINT_PTR task_ptr = (UINT_PTR)task.get();
    item->SetTag(task_ptr);
    CControlUI* ctrl = NULL;
    ctrl = item->FindSubControl(L"lbl_name");
    if (ctrl) {
        ctrl->SetText(task->title.c_str());
        ctrl->SetToolTip(task->title.c_str());
    }
    ctrl = item->FindSubControl(L"lbl_icon");
    if (ctrl) {
        if (PathFileExists(task->img.c_str()))
            ctrl->SetBkImage(task->img.c_str());
    }
    ctrl = item->FindSubControl(L"lbl_state");
    if (ctrl)
        ctrl->SetText(L"正在查询下载信息");
    //绑定消息处理
    item->OnNotify += MakeDelegate(this, &WndMain::OnNotifyListItem);
    list_loading_->Add(item);
    map_loading_items_.insert(std::make_pair(task_ptr, item));
    // OnListItemCountChange();
    if (opt_selall_loading->IsSelected()) {
        opt_selall_loading->Selected(false);
    }
    return true;
}

bool WndMain::AddFinishItem(const shared_ptr<download::Task>& task)
{
    CDialogBuilder builder;
    auto item = dynamic_cast<CListContainerElementUI*>(builder.Create(L"item_finish.xml", (LPCTSTR)0, this, &m_pm));
    assert(item);
    UINT_PTR task_ptr = (UINT_PTR)task.get();
    item->SetTag(task_ptr);
    CControlUI* ctrl = NULL;
    ctrl = item->FindSubControl(L"lbl_name");
    if (ctrl) {
        ctrl->SetText(task->title.c_str());
        ctrl->SetToolTip(task->title.c_str());
    }
    ctrl = item->FindSubControl(L"lbl_icon");
    if (ctrl) {
        if (PathFileExists(task->img.c_str()))
            ctrl->SetBkImage(task->img.c_str());
    }
    ctrl = item->FindSubControl(L"lbl_size");
    if (ctrl) {
        ctrl->SetText(string_utils::SecondsToString(task->duration).c_str());
    }
    ctrl = item->FindSubControl(L"lbl_date");
    if (ctrl) {
        ctrl->SetText(string_utils::TimeStampToString(task->ctime, false).c_str());
    }
    list_finish_->Add(item);
    return true;
}

bool WndMain::OnNotifySelectLoadingListItem(void* param)
{
    TNotifyUI* notify = reinterpret_cast<TNotifyUI*>(param);
    if (notify->sType == DUI_MSGTYPE_SELECTCHANGED) {

    }
    return true;
}

void WndMain::OnClickStartLoading(CControlUI* sender)
{
    auto item = sender->GetParent();
    assert(item);
    DOWNLOAD_SERVICE()->ReloadTask(item->GetTag());
}

void WndMain::OnClickStopLoading(CControlUI* sender)
{
    auto item = sender->GetParent();
    assert(item);
    DOWNLOAD_SERVICE()->StopTask(item->GetTag());
}

void WndMain::OnClickDeleteLoading(CControlUI* sender)
{
    ::PostMessage(m_hWnd, WM_MAINWND_MSGBOX, WPARAM_DELETE_LIST_ITEM, (LPARAM)sender);
}

void WndMain::OnClickFinishLoading(CControlUI* sender)
{
    ::PostMessage(m_hWnd, WM_MAINWND_MSGBOX, WPARAM_DELETE_LIST_ITEM, (LPARAM)sender);
}

bool WndMain::OnNotifyListItem(void* param)
{
    TNotifyUI* notify = reinterpret_cast<TNotifyUI*>(param);

    return true;
}

void WndMain::ShowLogin()
{
    if (!wnd_login_) {
        wnd_login_ = std::make_unique<WndLogin>();
        hwnd_login_ = wnd_login_->Create(m_hWnd);
    }
    wnd_login_->CenterWindow();
    wnd_login_->ShowModal();
    wnd_login_ = nullptr;
}

UINT WndMain::ShowMsgBox(MessageIconType icon, LPCWSTR info)
{
    WndMsg* wnd = new WndMsg(icon, info);
    wnd->Create(m_hWnd);
    wnd->CenterWindow();
    return wnd->ShowModal();
}

LRESULT WndMain::OnMsgMsgbox(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case MsgWparam::WPARAM_DELETE_LIST_ITEM:
        OnWparamDeleteItem(lParam);
        break;
    default:
        break;
    }
    return 0;
}

LRESULT WndMain::OnWparamDeleteItem(LPARAM lParam)
{
    UINT result = ShowMsgBox(MessageIconType::ICON_ASK, L"确定删除？");
    if (result == IDOK) {
        auto sender = reinterpret_cast<CControlUI*>(lParam);
        assert(sender);
        auto item = sender->GetParent();
        assert(item);
        UINT_PTR task_id = (UINT_PTR)lParam;
        if (list_loading_->GetItemIndex(item) > -1) {
            DOWNLOAD_SERVICE()->DeleteLoadingTask(task_id);
            list_loading_->Remove(item);
        }
        else {
            DOWNLOAD_SERVICE()->DeleteFinishTask(task_id);
            list_finish_->Remove(item);
        }
    }
    return 0;
}

LRESULT WndMain::OnMsgDecode(WPARAM wParam, LPARAM lParam)
{
    UINT_PTR task_id = (UINT_PTR)wParam;
    DecodeErrorCode code = static_cast<DecodeErrorCode>(lParam);
    auto iter = map_loading_items_.find(task_id);
    if (iter == map_loading_items_.end()) {
        LOG(ERROR) << "NotifyStatus 未找到下载节点";
        return -1;
    }
    auto task = DOWNLOAD_SERVICE()->FindLoadingTask(task_id);
    if (task == nullptr) {
        LOG(ERROR) << "OnMsgNotifyStatus 没有找到下载对象";
        return -1;
    }
    if (code == DecodeErrorCode::ERROR_SUCCESS) {
        task->status = download::DownloadStatus::STATUS_FINISH;
    }
    else {
        task->status = download::DownloadStatus::STATUS_FAILED;
    }
    // 更新状态
    ::PostMessage(m_hWnd, WM_MAINWND_NOTIFY_STATUS, (WPARAM)task_id, (LPARAM)0);
    return 0;
}

LRESULT WndMain::OnMsgNotifyStatus(WPARAM wParam, LPARAM lParam)
{
    UINT_PTR task_id = (UINT_PTR)wParam;
    void* param = (void*)lParam;
    auto task = DOWNLOAD_SERVICE()->FindLoadingTask(task_id);
    if (task == nullptr) {
        LOG(ERROR) << "OnMsgNotifyStatus 没有找到下载对象";
        return -1;
    }
    NotifyStatus(task, param);
    return 0;
}
