#pragma once
#include "common_define.h"
#include "async_service_define.h"

class IAsyncServiceDelegate {
public:
    virtual void OnAsyncComplete(AsyncTaskType task_type, AsyncErrorCode code, void* data, void* param) = 0;
};