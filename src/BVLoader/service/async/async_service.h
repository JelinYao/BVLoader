#pragma once
#include <list>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "common_define.h"
#include "Service.h"
#include "async_service_define.h"

class IAsyncTask;
class VideoTask;
class IAsyncServiceDelegate;
class AsyncService
    : public IAsyncService
{
public:
    AsyncService();
    ~AsyncService();

    void AddDelegate(IAsyncServiceDelegate* delegate, void* param) override;
    void RemoveDelegate(IAsyncServiceDelegate* delegate) override;
    void AddHttpTask(AsyncTaskType task_type, std_cstr_ref url) override;
    void AddVideoTask(AsyncTaskType task_type, VideoType video_type, std_cstr_ref url) override;
    void AddDecodeTask(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) override;
    void AddLoginTask(std_cstr_ref url, std_cstr_ref auth_key) override;
    void AddDownloadImageTask(ImageType image_type, std_cstr_ref url) override;

protected:
    bool Init() override;
    void Exit() override;
    void InitHttp(RestClient::Connection& http);

    // 异步处理线程
    static void ThreadProc(void* param);
    void DoAsyncWork();
    void OnGetVideoInfo(const std::shared_ptr<IAsyncTask>& task);
    void OnDownloadImage(const std::shared_ptr<IAsyncTask>& task);
    void OnGetPlayerUrl(const std::shared_ptr<IAsyncTask>& task);
    void OnDecodeVideo(const std::shared_ptr<IAsyncTask>& task);
    void OnGetLoginUrl(const std::shared_ptr<IAsyncTask>& task);
    void OnGetLoginInfo(const std::shared_ptr<IAsyncTask>& task);
    void OnGetUserInfo(const std::shared_ptr<IAsyncTask>& task);
    std_str DoHttpRequest(const std::shared_ptr<IAsyncTask>& task);
    // 数据解析
    bool ParseUgcInfo(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response);
    bool ParseUgcPlayerUrl(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response);
    bool ParseLoginUrl(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response);
    bool ParseLoginInfo(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response);
    bool ParseUserInfo(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response);

    void NotifyDelegate(AsyncTaskType task_type, AsyncErrorCode code, void* data);
    void ClearDelegate();
    bool IsDelegateEmpty() {
        std::lock_guard<std::mutex> lock(delegate_mutex_);
        return delegate_list_.empty();
    }

private:
    bool need_exit_ = false;
    std::mutex task_mutex_;
    std::condition_variable task_event_;
    std::unique_ptr<std::thread> async_thread_;
    std::list<std::shared_ptr<IAsyncTask>> task_list_;
    std::mutex delegate_mutex_;
    std::map<IAsyncServiceDelegate*, void*> delegate_list_;
    std_str user_cookie_;
};

