#pragma once

// 视频类型
enum class VideoType {
    VIDEO_UNKNOWN = 0,
    VIDEO_UGC,
};

// 异步任务类型
enum class AsyncTaskType {
    TASK_NONE = 0,
    TASK_GET_INFO, // 获取详细信息
    TASK_GET_PLAYER_URL, // 获取视频下载地址
    TASK_GET_SELECT_PLAYER_URL, // 获取视频下载地址
    TASK_DOWNLOAD_COVER, // 下载封面
    TASK_DECODE_VIDEO, // 视频解码
    TASK_GET_LOGIN_URL, // 登录二维码链接
    TASK_GET_LOGIN_INFO, // 登录二维码结果
};

static const std::string kDeafultReferer = "https://www.bilibili.com";
static const std::string kDefaultUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36";
constexpr int kDefaultHttpTimeOut = 20;

// http response key word
static constexpr const char* kHttpResponseCode = "code";
static constexpr const char* kHttpResponseMessage = "message";
static constexpr const char* kHttpResponseData = "data";