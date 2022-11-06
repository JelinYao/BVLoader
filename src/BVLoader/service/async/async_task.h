#pragma once
#include "common_define.h"

class IAsyncTask 
{
public:
    IAsyncTask(AsyncTaskType type)
        : task_type(type) {
    }

    virtual ~IAsyncTask() {
    }

    AsyncTaskType task_type;
};

class HttpTask
    : public IAsyncTask 
{
public:
    HttpTask(AsyncTaskType type)
        : IAsyncTask(type) {
    }

    HttpTask(AsyncTaskType type, std_cstr_ref u)
        : IAsyncTask(type)
        , url(u) {
    }

    ~HttpTask() {
    }

    std_str url;
};

class VideoTask
    : public HttpTask
{
public:
    VideoTask(AsyncTaskType type)
        : HttpTask(type) {
    }

    VideoTask(AsyncTaskType type, VideoType vt, std_cstr_ref url)
        : HttpTask(type, url)
        , video_type(vt) {
    }

    virtual ~VideoTask() {
    }

    VideoType video_type;
};

class DecodeTask
    : public IAsyncTask 
{
public:
    DecodeTask(UINT_PTR id, std_cwstr_ref video, std_cwstr_ref mp3, std_cwstr_ref img)
        : IAsyncTask(AsyncTaskType::TASK_DECODE_VIDEO)
        , task_id(id)
        , video_path(video)
        , mp3_path(mp3)
        , img_path(img) {
    }

    std_wstr video_path;
    std_wstr mp3_path;
    std_wstr img_path;
    UINT_PTR task_id = 0;
};

class LoginTask
    : public HttpTask
{
public:
    LoginTask(std_cstr_ref url, std_cstr_ref key)
        : HttpTask(AsyncTaskType::TASK_GET_LOGIN_INFO, url)
        , auth_key(key) {
    }

    std_str auth_key;
};

class ImageTask 
    : public HttpTask {
public:
    ImageTask(ImageType type, std_cstr_ref url)
        : HttpTask(AsyncTaskType::TASK_DOWNLOAD_IMAGE, url)
        , image_type(type) {
    }

    ImageType image_type;
};