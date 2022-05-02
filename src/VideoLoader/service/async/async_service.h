#pragma once
#include <list>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "common_define.h"
#include "Service.h"

class IAsyncTask;
class HttpTask;
class IAsyncServiceDelegate;
class AsyncService
    : public IAsyncService
{
public:
    AsyncService();
    ~AsyncService();

    void AddDelegate(IAsyncServiceDelegate* delegate, void* param) override;
    void AddHttpTask(AsyncTaskType task_type, VideoType video_type, std_cstr_ref url) override;
    void AddDecodeTask(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) override;

protected:
    bool Init() override;
    void Exit() override;

    // 异步处理线程
    static void ThreadProc(void* param);
    void DoAsyncWork();
    void OnGetVideoInfo(const std::shared_ptr<IAsyncTask>& task);
    void OnDownloadCover(const std::shared_ptr<IAsyncTask>& task);
    void OnGetPlayerUrl(const std::shared_ptr<IAsyncTask>& task);
    void OnDecodeVideo(const std::shared_ptr<IAsyncTask>& task);
    std_str DoHttpRequest(const std::shared_ptr<HttpTask>& task);
    // 数据解析
    bool ParseUgcInfo(const std::shared_ptr<HttpTask>& task, std_cstr_ref response);
    bool ParseUgcPlayerUrl(const std::shared_ptr<HttpTask>& task, std_cstr_ref response);

private:
    bool need_exit_ = false;
    std::mutex task_mutex_;
    std::condition_variable task_event_;
    std::unique_ptr<std::thread> async_thread_;
    std::list<std::shared_ptr<IAsyncTask>> task_list_;
    IAsyncServiceDelegate* delegate_ = nullptr;
};

