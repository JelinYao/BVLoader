#pragma once
#include <list>
#include <memory>
#include "task.h"

namespace download {
    class TaskManager {
    public:
        TaskManager();
        ~TaskManager();
        std::shared_ptr<Task> NewTask(std_cstr_ref url, std_cwstr_ref title, std_cwstr_ref img, 
            std_cwstr_ref author, int duration, __int64 ctime);
        void FreeTask(std::shared_ptr<Task>&& task);
        void FreeTask(const std::shared_ptr<Task>& task);

        void AddLoadigTask(const std::shared_ptr<Task>& task);
        std::shared_ptr<Task> FindLoadingTask(Task* task_ptr);
        void DeleteLoadingTask(const std::shared_ptr<Task>& task);
        int GetLoadingCount();

        void AddFinishTask(const std::shared_ptr<Task>& task);
        void AddFinishTask(std::shared_ptr<Task>&& task);
        std::shared_ptr<Task> FindFinishTask(Task* task_ptr);
        void DeleteFinishTask(const std::shared_ptr<Task>& task);

    protected:
        void Reserve(int count);

    private:
        std::list<std::shared_ptr<Task>> loading_list_;
        std::list<std::shared_ptr<Task>> finish_list;
        std::list<std::shared_ptr<Task>> free_list_;
    };
}// namespace download