#include "pch.h"
#include "task_manager.h"

namespace download {
    static constexpr int kFreeListReserveSize = 10;
    TaskManager::TaskManager()
    {
        Reserve(kFreeListReserveSize);
    }

    TaskManager::~TaskManager()
    {
    }

    void TaskManager::AddLoadigTask(const std::shared_ptr<Task>& task)
    {
        loading_list_.push_back(task);
    }

    int TaskManager::GetLoadingCount()
    {
        int count = 0;
        for (auto& task : loading_list_) {
            if (task->status == DownloadStatus::STATUS_LOADING) {
                count++;
            }
        }
        return count;
    }

    std::shared_ptr<download::Task> TaskManager::FindLoadingTask(Task* task_ptr)
    {
        for (auto& task : loading_list_) {
            if (task.get() == task_ptr) {
                return task;
            }
        }
        return nullptr;
    }

    std::shared_ptr<download::Task> TaskManager::FindFinishTask(Task* task_ptr)
    {
        for (auto& task : finish_list) {
            if (task.get() == task_ptr) {
                return task;
            }
        }
        return nullptr;
    }

    void TaskManager::Reserve(int count)
    {
        assert(count > 0);
        for (int i = 0; i < count; ++i) {
            free_list_.emplace_back(std::make_shared<Task>());
        }
    }

    std::shared_ptr<download::Task> TaskManager::NewTask(std_cstr_ref url, std_cwstr_ref title, std_cwstr_ref img, 
        std_cwstr_ref author, int duration, __int64 ctime)
    {
        if (free_list_.empty()) {
            Reserve(kFreeListReserveSize);
        }
        auto task = *free_list_.begin();
        free_list_.pop_front();
        task->url = url;
        task->title = title;
        task->author = author;
        task->img = img;
        task->duration = duration;
        task->ctime = ctime;
        return task;
    }

    void TaskManager::FreeTask(std::shared_ptr<Task>&& task)
    {
        free_list_.emplace_back(std::move(task));
    }

    void TaskManager::FreeTask(const std::shared_ptr<Task>& task)
    {
        free_list_.push_back(task);
    }

    void TaskManager::DeleteLoadingTask(const std::shared_ptr<Task>& task)
    {
        auto iter = std::find(loading_list_.begin(), loading_list_.end(), task);
        if (iter == loading_list_.end()) {
            return;
        }
        auto temp_task = *iter;
        loading_list_.erase(iter);
        free_list_.emplace_back(std::move(temp_task));
    }

}// namespace download