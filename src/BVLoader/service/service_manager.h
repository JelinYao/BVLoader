#pragma once
#include "Service.h"

#define ASYNC_SERVICE() ServiceManager::Instance()->GetAsyncService()
#define DOWNLOAD_SERVICE() ServiceManager::Instance()->GetDownloadService()
#define AUDIO_SERVICE() ServiceManager::Instance()->GetAudioService()
class ServiceManager
{
public:
    static ServiceManager* Instance() {
        static ServiceManager mgr;
        return &mgr;
    }

    bool Init(HINSTANCE instance);
    void Exit();

    std::shared_ptr<IAsyncService> GetAsyncService();
    std::shared_ptr<IDownloadService> GetDownloadService();
    std::shared_ptr<IAudioService> GetAudioService();

protected:
    ServiceManager();
    ~ServiceManager();

private:
    std::shared_ptr<IAsyncService> async_service_;
    std::shared_ptr<IDownloadService> download_service_;
    std::shared_ptr<IAudioService> audio_service_;
};

