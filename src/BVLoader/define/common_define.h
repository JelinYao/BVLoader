#pragma once

// ��Ƶ����
enum class VideoType {
    VIDEO_UNKNOWN = 0,
    VIDEO_UGC,
};

// �첽��������
enum class AsyncTaskType {
    TASK_NONE = 0,
    TASK_GET_INFO, // ��ȡ��ϸ��Ϣ
    TASK_GET_PLAYER_URL, // ��ȡ��Ƶ���ص�ַ
    TASK_GET_SELECT_PLAYER_URL, // ��ȡ��Ƶ���ص�ַ
    TASK_DOWNLOAD_COVER, // ���ط���
    TASK_DECODE_VIDEO, // ��Ƶ����
    TASK_GET_LOGIN_URL, // ��¼��ά������
    TASK_GET_LOGIN_INFO, // ��¼��ά����
};

static const std::string kDeafultReferer = "https://www.bilibili.com";
static const std::string kDefaultUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36";
constexpr int kDefaultHttpTimeOut = 20;

// http response key word
static constexpr const char* kHttpResponseCode = "code";
static constexpr const char* kHttpResponseMessage = "message";
static constexpr const char* kHttpResponseData = "data";