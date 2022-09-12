#include "pch.h"
#include "download_service.h"
#include "download_service_define.h"
#include "task_manager.h"
#include "http_cache.h"

namespace download {

    HWND DownloadService::msg_wnd_ = NULL;
    DownloadService::DownloadService(HINSTANCE instance)
        : instance_(instance)
    {
    }

    DownloadService::~DownloadService()
    {
    }

    bool DownloadService::Init()
    {
        task_mgr_ = std::make_shared<TaskManager>();
        http_cache_ = std::make_shared<HttpCache>();
        msg_wnd_ = CreateMsgWindow();
        if (msg_wnd_ == NULL) {
            LOG(ERROR) << "Init Create message window failed, error code: " << ::GetLastError();
            return false;
        }
        ::SetWindowLongPtr(msg_wnd_, 0, (LONG)this);
        wchar_t path[MAX_PATH +1] = { 0 };
        system_utils::GetExePathW(path, MAX_PATH);
        download_path_.assign(path);
        download_path_.append(L"download\\");
        if (!PathFileExists(download_path_.c_str()) && !CreateDirectory(download_path_.c_str(), NULL)) {
            MessageBox(NULL, L"创建下载目录失败，权限不足", L"出错了：", MB_OK | MB_ICONERROR);
            return false;
        }
        audio_path_ = download_path_ + L"audio\\";
        if (!PathFileExists(audio_path_.c_str()) && !CreateDirectory(audio_path_.c_str(), NULL)) {
            MessageBox(NULL, L"创建下载目录失败，权限不足", L"出错了：", MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }

    void DownloadService::Exit()
    {
        delegate_list_.clear();
        ::SetWindowLongPtr(msg_wnd_, 0, (LONG)0);
        if (msg_wnd_) {
            ::CloseWindow(msg_wnd_);
            ::DestroyWindow(msg_wnd_);
            msg_wnd_ = NULL;
        }
        http_cache_->Exit();
    }

    void DownloadService::AddDelegate(download::IDownloadServiceDelegate* delegate, void* param)
    {
        assert(delegate);
        delegate_list_.emplace(delegate, param);
    }

    HWND DownloadService::CreateMsgWindow()
    {
        static const wchar_t kWndClass[] = L"ClientMessageWindow";
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = MsgWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(LONG_PTR) * 2;
        wc.hInstance = instance_;
        wc.lpszClassName = kWndClass;
        RegisterClassEx(&wc);
        return ::CreateWindow(kWndClass, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, instance_, 0);
    }

    LRESULT DownloadService::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_MSG_UPDATE_TASK_PROGRESS:
            OnMsgUpdateTaskProgress(wParam, lParam);
            break;
        case WM_MSG_UPDATE_TASK_STATUS:
            OnMsgUpdateTaskStatus(wParam, lParam);
            break;
        default:
            break;
        }
        return 0;
    }

    LRESULT DownloadService::OnMsgUpdateTaskProgress(WPARAM wParam, LPARAM lParam)
    {
        Task* task_ptr = reinterpret_cast<Task*>(lParam);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "OnMsgUpdateTaskProgress 获取任务对象失败";
            return -1;
        }
        auto task = task_mgr_->FindLoadingTask(task_ptr);
        if (task == nullptr) {
            LOG(ERROR) << "OnMsgUpdateTaskProgress 获取任务智能指针对象失败";
            return -1;
        }
        int speed = (int)wParam;
        NotifyProgress(task, speed);
        return 0;
    }

    LRESULT DownloadService::OnMsgUpdateTaskStatus(WPARAM wParam, LPARAM lParam)
    {
        Task* task_ptr = reinterpret_cast<Task*>(lParam);
        if (task_ptr == nullptr) {
            LOG(ERROR) << "OnMsgUpdateTaskStatus 获取任务对象失败";
            return -1;
        }
        auto task = task_mgr_->FindLoadingTask(task_ptr);
        if (task == nullptr) {
            LOG(ERROR) << "OnMsgUpdateTaskStatus 获取任务智能指针对象失败";
            return -1;
        }
        httplib::DownloadState state = (httplib::DownloadState)wParam;
        switch (state)
        {
        case httplib::STATE_DOWNLOAD_HAS_FAILED:
            OnTaskLoadFailed(task);
            break;
        case httplib::STATE_DOWNLOAD_HAS_FINISHED:
            OnTaskLoadSuccess(task);
            break;
        default:
            break;
        }
        return 0;
    }

    LRESULT DownloadService::MsgWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        DownloadService* service = reinterpret_cast<DownloadService*>(::GetWindowLongPtr(hWnd, 0));
        if (service) {
            return service->HandleMessage(hWnd, message, wParam, lParam);
        }
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    void DownloadService::LoadCallback(httplib::DownloadState state, double dltotal, double dlnow, void* user_data)
    {
        if (!::IsWindow(msg_wnd_)) {
            return;
        }
        Task* task = reinterpret_cast<Task*>(user_data);
        if (task == nullptr) {
            LOG(ERROR) << "LoadCallback invalid Task";
            return;
        }
        switch (state)
        {
        case httplib::STATE_DOWNLOADING: {
            // 下载中
            if (dltotal < 1.0f) {
                return;
            }
            if (task->total_size == 0) {
                task->total_size = dltotal;
            }
            double add_size = dlnow - task->load_size;
            task->load_size = dlnow;
            __int64 stamp = time(NULL);
            if (task->ltime == 0 || stamp == task->ltime) {
                task->ltime = stamp;
                task->temp_size += add_size;
                return;
            }
            // 计算下载速度
            int speed = (int)((task->temp_size / (stamp - task->ltime)) / 1024);
            task->temp_size = 0.0F;
            task->ltime = stamp;
            // 更新下载状态
            ::PostMessage(msg_wnd_, WM_MSG_UPDATE_TASK_PROGRESS, (WPARAM)speed, (LPARAM)task);
            break;
        }
        case httplib::STATE_DOWNLOAD_HAS_FAILED:
        case httplib::STATE_DOWNLOAD_HAS_STOPED:
        case httplib::STATE_DOWNLOAD_HAS_FINISHED:
            ::PostMessage(msg_wnd_, WM_MSG_UPDATE_TASK_STATUS, (WPARAM)state, (LPARAM)task);
            break;
        default:
            break;
        }
        
    }

    bool DownloadService::StartTask(const std::shared_ptr<Task>& task)
    {
        if (task->url.empty() || task->save_path.empty()) {
            LOG(ERROR) << "下载任务还未初始化";
            return false;
        }
        if (PathFileExists(task->save_path.c_str())) {
            // 文件下载完成
            OnTaskLoadSuccess(task);
            return true;
        }
        if (task_mgr_->GetLoadingCount() < max_loading_count_) {
            task->http = std::make_shared<httplib::CHttpDownload>();
            if (!task->http->Initialize(task->url, task->save_path)) {
                LOG(ERROR) << "StartTask 初始化下载库失败";
                task->status = DownloadStatus::STATUS_FAILED;
                task->http = nullptr;
                NotifyStatus(task);
                return false;
            }
            task->http->AddHeader("Referer", kDeafultReferer);
            task->http->SetProcessCallback(LoadCallback, task.get());
            task->http->Start();
            task->status = DownloadStatus::STATUS_LOADING;
        }
        else {
            task->status = DownloadStatus::STATUS_WAITTING;
        }
        NotifyStatus(task);
        return true;
    }

    void DownloadService::OnTaskLoadSuccess(const std::shared_ptr<Task>& task)
    {
        task->status = DownloadStatus::STATUS_DOWNLOAD_SUCCESS;
        ClearTaskHttp(task);
        NotifyStatus(task);
    }

    void DownloadService::OnTaskLoadFailed(const std::shared_ptr<Task>& task)
    {
        task->status = DownloadStatus::STATUS_FAILED;
        ClearTaskHttp(task);
        NotifyStatus(task);
    }

    void DownloadService::NotifyStatus(const std::shared_ptr<Task>& task)
    {
        for (auto& info : delegate_list_) {
            info.first->NotifyStatus(task, info.second);
        }
    }

    void DownloadService::NotifyProgress(const std::shared_ptr<Task>& task, int speed)
    {
        for (auto& info : delegate_list_) {
            info.first->NotifyProgress(task, info.second, speed);
        }
    }

    std::shared_ptr<Task> DownloadService::AddTask(std_cstr_ref url, std_cwstr_ref title, std_cwstr_ref img,
        std_cwstr_ref author, int duration, __int64 ctime, std_cwstr_ref name/* = L""*/)
    {
        auto task = task_mgr_->NewTask(url, title, img, author, duration, ctime);
        std::wstring save_name = name;
        if (save_name.empty()) {
            auto file_name = string_utils::GetFileNameByUrl(url);
            if (file_name.empty()) {
                LOG(ERROR) << "AddTask 获取下载文件名失败，url: " << url;
                task_mgr_->FreeTask(std::move(task));
                return nullptr;
            }
            save_name = string_utils::Utf8ToU(file_name);
        }
        else {
            save_name += string_utils::Utf8ToU(string_utils::GetFileExtentionNameByUrl(url));
        }
        
        task->save_path = download_path_ + save_name;
        task->audio_path = audio_path_ + save_name + L".mp3";
        task_mgr_->AddLoadigTask(task);
        NotifyStatus(task);
        StartTask(task);
        return task;
    }

    bool DownloadService::StopTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        auto task = task_mgr_->FindLoadingTask(task_ptr);
        if (!task) {
            LOG(ERROR) << "StopTask 任务不存在";
            return false;
        }
        if (task->status != DownloadStatus::STATUS_LOADING) {
            LOG(ERROR) << "StopTask 任务状态不正确，status: " << (int)task->status;
            return false;
        }
        ClearTaskHttp(task);
        task->status = DownloadStatus::STATUS_PAUSE;
        NotifyStatus(task);
        return true;
    }

    bool DownloadService::ReloadTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        auto task = task_mgr_->FindLoadingTask(task_ptr);
        if (!task) {
            LOG(ERROR) << "ReloadTask 任务不存在";
            return false;
        }
        if (task->status != DownloadStatus::STATUS_PAUSE 
            && task->status != DownloadStatus::STATUS_FAILED) {
            LOG(ERROR) << "ReloadTask 任务状态不正确，status: " << (int)task->status;
            return false;
        }
        StartTask(task);
        return true;
    }

    bool DownloadService::DeleteLoadingTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        auto task = task_mgr_->FindLoadingTask(task_ptr);
        if (!task) {
            LOG(ERROR) << "DeleteLoadingTask 任务不存在";
            return false;
        }
        ClearTaskHttp(task);
        task->Clear();
        task_mgr_->DeleteLoadingTask(task);
        return true;
    }

    bool DownloadService::DeleteFinishTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        auto task = task_mgr_->FindFinishTask(task_ptr);
        if (!task) {
            LOG(ERROR) << "DeleteFinishTask 任务不存在";
            return false;
        }
        ClearTaskHttp(task);
        task->Clear();
        task_mgr_->DeleteFinishTask(task);
        return true;
    }

    bool DownloadService::AddFinishTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        auto task = task_mgr_->FindLoadingTask(task_ptr);
        if (!task) {
            LOG(ERROR) << "AddFinishTask 任务不存在";
            return false;
        }
        task_mgr_->AddFinishTask(std::move(task));
        return true;
    }

    std_cwstr_ref DownloadService::GetDownloadPath() const
    {
        return download_path_;
    }

} // namespace download

