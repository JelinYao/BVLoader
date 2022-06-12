#pragma once
#include "service.h"
#include "task.h"
#include <utils/HttpLib.h>
#include "download_service_delegate.h"
#include <list>

namespace download {
    class TaskManager;
    class HttpCache;
    class DownloadService 
    : public IDownloadService
    {
    public:
        DownloadService(HINSTANCE instance);
        ~DownloadService();

        void AddDelegate(download::IDownloadServiceDelegate* delegate, void* param) override;
        std::shared_ptr<Task> AddTask(std_cstr_ref url, std_cwstr_ref title, std_cwstr_ref img,
            std_cwstr_ref author, int duration, __int64 ctime, std_cwstr_ref name = L"") override;
        bool StopTask(UINT_PTR task_id) override;
        bool ReloadTask(UINT_PTR task_id) override;
        bool DeleteLoadingTask(UINT_PTR task_id) override;
        bool DeleteFinishTask(UINT_PTR task_id) override;
        std::shared_ptr<download::Task> FindLoadingTask(UINT_PTR task_id) override;
        std::shared_ptr<download::Task> FindFinishTask(UINT_PTR task_id) override;
        bool AddFinishTask(UINT_PTR task_id) override;

    protected:
        bool Init() override;
        void Exit() override;
        // message window
        HWND CreateMsgWindow();
        LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT OnMsgUpdateTaskProgress(WPARAM wParam, LPARAM lParam);
        LRESULT OnMsgUpdateTaskStatus(WPARAM wParam, LPARAM lParam);

        static LRESULT WINAPI MsgWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        static void LoadCallback(httplib::DownloadState state, double dltotal, double dlnow, void* user_data);

        bool StartTask(const std::shared_ptr<Task>& task);
        void OnTaskLoadSuccess(const std::shared_ptr<Task>& task);
        void OnTaskLoadFailed(const std::shared_ptr<Task>& task);
        void NotifyStatus(const std::shared_ptr<Task>& task);
        void NotifyProgress(const std::shared_ptr<Task>& task, int speed);
        void ClearTaskHttp(const std::shared_ptr<Task>& task);

    private:
        static HWND msg_wnd_;
        int max_loading_count_ = 6;
        HINSTANCE instance_ = NULL;
        std::shared_ptr<TaskManager> task_mgr_;
        std_wstr download_path_;
        std_wstr audio_path_;
        std::shared_ptr <HttpCache> http_cache_;
        std::unordered_map<IDownloadServiceDelegate*, void*> delegate_list_;
    };
} // namespace download

#include "download_service.inl"