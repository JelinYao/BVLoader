#pragma once
#include "common_define.h"
#include "service/download/task.h"

class IService {
public:
    virtual bool Init() = 0;
    virtual void Exit() = 0;
};

class IAsyncServiceDelegate;
class IAsyncService 
    : public IService {
public:
    virtual void AddDelegate(IAsyncServiceDelegate* delegate, void* param) = 0;
    virtual void AddHttpTask(AsyncTaskType task_type, VideoType video_type, std_cstr_ref url) = 0;
    virtual void AddDecodeTask(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) = 0;
};

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
    virtual std::shared_ptr<download::Task> FindTask(UINT_PTR task_id) = 0;
};

class IAudioServiceDelegate;
class IAudioService
    : public IService {
public:
    virtual void AddDelegate(IAudioServiceDelegate* delegate, void* param) = 0;
    virtual void Decode(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) = 0;
};