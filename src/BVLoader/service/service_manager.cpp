#include "pch.h"
#include "service_manager.h"
#include "async/async_service.h"
#include "download/download_service.h"
#include "audio/audio_service.h"

ServiceManager::ServiceManager()
{
}

ServiceManager::~ServiceManager()
{
}

bool ServiceManager::Init(HINSTANCE instance)
{
    async_service_ = std::make_shared<AsyncService>();
    if (!async_service_->Init()) {
        LOG(ERROR) << "Init async service failed";
        return false;
    }
    download_service_ = std::make_shared<download::DownloadService>(instance);
    if (!download_service_->Init()) {
        LOG(ERROR) << "Init download service failed";
        return false;
    }
    audio_service_ = std::make_shared<AudioService>();
    if (!audio_service_->Init()) {
        LOG(ERROR) << "Init audio service failed";
        return false;
    }
    return true;
}

void ServiceManager::Exit()
{
    if (async_service_) {
        async_service_->Exit();
    }
    if (download_service_) {
        download_service_->Exit();
    }
    if (audio_service_) {
        audio_service_->Exit();
    }
}

std::shared_ptr<IAsyncService> ServiceManager::GetAsyncService()
{
    return async_service_;
}

std::shared_ptr<IDownloadService> ServiceManager::GetDownloadService()
{
    return download_service_;
}

std::shared_ptr<IAudioService> ServiceManager::GetAudioService()
{
    return audio_service_;
}

