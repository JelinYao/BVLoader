#pragma once
#include "common_define.h"
#include "service/download/task.h"

class IService {
public:
    virtual bool Init() = 0;
    virtual void Exit() = 0;
};

// 异步服务对外能力接口
class IAsyncServiceDelegate;
class IAsyncService 
    : public IService {
public:
    virtual void AddDelegate(IAsyncServiceDelegate* delegate, void* param) = 0;
    virtual void RemoveDelegate(IAsyncServiceDelegate* delegate) = 0;
    virtual void AddHttpTask(AsyncTaskType task_type, std_cstr_ref url) = 0;
    virtual void AddVideoTask(AsyncTaskType task_type, VideoType video_type, std_cstr_ref url) = 0;
    virtual void AddDecodeTask(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) = 0;
    virtual void AddLoginTask(std_cstr_ref url, std_cstr_ref auth_key) = 0;
    virtual void AddDownloadImageTask(ImageType image_type, std_cstr_ref url) = 0;
    virtual void SetCookie(std_cstr_ref cookie) = 0;
};

// 下载服务对外能力接口
namespace download {
    class IDownloadServiceDelegate;
}
class IDownloadService
    : public IService {
public:
    virtual void AddDelegate(download::IDownloadServiceDelegate* delegate, void* param) = 0;
    virtual std::shared_ptr<download::Task> AddTask(std_cstr_ref url, std_cwstr_ref title, std_cwstr_ref img, 
        std_cwstr_ref author, int duration, __int64 ctime) = 0;
    virtual bool StopTask(UINT_PTR task_id) = 0;
    virtual bool ReloadTask(UINT_PTR task_id) = 0;
    virtual bool DeleteLoadingTask(UINT_PTR task_id) = 0;
    virtual bool DeleteFinishTask(UINT_PTR task_id) = 0;
    virtual std::shared_ptr<download::Task> FindLoadingTask(UINT_PTR task_id) = 0;
    virtual std::shared_ptr<download::Task> FindFinishTask(UINT_PTR task_id) = 0;
    virtual bool AddFinishTask(UINT_PTR task_id) = 0;
};

// 解码服务对外能力接口
class IAudioServiceDelegate;
class IAudioService
    : public IService {
public:
    virtual void AddDelegate(IAudioServiceDelegate* delegate, void* param) = 0;
    virtual void Decode(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) = 0;
};