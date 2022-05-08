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

    HttpTask(AsyncTaskType type, VideoType vt, std_cstr_ref u)
        : IAsyncTask(type)
        , video_type(vt)
        , url(u) {
    }

    virtual ~HttpTask() {
    }

    std_str url;
    VideoType video_type;
};

class DecodeTask
    : public IAsyncTask 
{
public:
    DecodeTask(UINT_PTR id, std_cwstr_ref video, std_cwstr_ref mp3)
        : IAsyncTask(AsyncTaskType::TASK_DECODE_VIDEO)
        , task_id(id)
        , video_path(video)
        , mp3_path(mp3) {
    }

    std_wstr video_path;
    std_wstr mp3_path;
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