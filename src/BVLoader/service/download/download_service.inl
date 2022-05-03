#pragma once
#include "http_cache.h"
#include "task_manager.h"

namespace download {

    inline void DownloadService::ClearTaskHttp(const std::shared_ptr<Task>& task)
    {
        if (task->http) {
            task->http->NeedStop();
            http_cache_->Add(std::move(task->http));
            task->http = nullptr;
        }
    }

    inline std::shared_ptr<download::Task> DownloadService::FindLoadingTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        if (task_ptr == nullptr) {
            return nullptr;
        }
        return task_mgr_->FindLoadingTask(task_ptr);
    }

    inline std::shared_ptr<download::Task> DownloadService::FindFinishTask(UINT_PTR task_id)
    {
        Task* task_ptr = reinterpret_cast<Task*>(task_id);
        if (task_ptr == nullptr) {
            return nullptr;
        }
        return task_mgr_->FindFinishTask(task_ptr);
    }
    
} // namespace download
