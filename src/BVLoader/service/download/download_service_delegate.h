#pragma once
#include "task.h"

namespace download {

    class IDownloadServiceDelegate {
    public:
        virtual void NotifyStatus(const std::shared_ptr<Task>& task, void* param) = 0;
        virtual void NotifyProgress(const std::shared_ptr<Task>& task, void* param, int speed) = 0;
    };
}// namespace download
