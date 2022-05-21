#pragma once

// 定义错误类型
#ifdef ERROR_SUCCESS
#undef ERROR_SUCCESS
#endif
enum class AsyncErrorCode {
    ERROR_SUCCESS = 0,
    ERROR_NETWORK_UNAVAILABLE,
    ERROR_PARSE_RESPONSE,
    ERROR_RESPONSE_CODE,
};

// 视频基础信息
class VideoInfo {
public:
    VideoInfo(std_str_r_ref _bvid, std_wstr_r_ref _title, std_wstr_r_ref _author,
        __int64 _aid, __int64 _cid, int _duration, __int64 _ctime)
        : bvid(std::move(_bvid)) 
        , title(std::move(_title))
        , author(std::move(_author))
        , aid(_aid)
        , cid(_cid)
        , duration(_duration)
        , ctime(_ctime) {
    }

    std_str bvid;
    std_wstr title;
    std_wstr author;
    __int64 aid = 0;
    __int64 cid = 0;
    int duration = 0;
    __int64 ctime = 0;
};

// 视频画质信息
class QualityInfo {
public:
    QualityInfo(int _value, std_str_r_ref _desc)
        : value(_value)
        , desc(std::move(_desc)) {
    }

    QualityInfo(const QualityInfo& obj) {
        value = obj.value;
        desc = obj.desc;
    }

    QualityInfo(QualityInfo&& obj) {
        value = obj.value;
        desc = std::move(obj.desc);
    }

    QualityInfo& operator=(QualityInfo&& obj) {
        if (&obj != this) {
            value = obj.value;
            desc = std::move(obj.desc);
        }
        return *this;
    }

    QualityInfo& operator=(const QualityInfo& obj) {
        value = obj.value;
        desc = obj.desc;
        return *this;
    }

    int value = 0;
    std_str desc;
};

// 视频播放信息
class PlayerInfo {
public:
    PlayerInfo(std_str_r_ref _url, std::vector<QualityInfo>&& _qn)
        : url(std::move(_url))
        , qn(std::move(_qn)) {
    }

    std_str url;
    std::vector<QualityInfo> qn;
};

// 登录二维码url信息
class QrcodeUrlInfo {
public:
    QrcodeUrlInfo(std_str_r_ref _url, std_str_r_ref _key)
        : url(std::move(_url))
        , auth_key(std::move(_key)) {
    }

    std_str url;
    std_str auth_key;
};

// 登录状态信息
class QrcodeLoginInfo {
public:
    QrcodeLoginInfo(int _code, std_str_r_ref _url, std_str_r_ref _cookie)
        : code(_code)
        , url(std::move(_url))
        , cookie(std::move(_cookie)) {
    }

    int code = 0;
    std_str url;
    std_str cookie;
};

// 用户信息
class UserInfo {
public:
    UserInfo() {
    }

    int current_level = 0;
    int current_min = 0;
    int current_exp = 0;
    int next_exp = 0;
    int money = 0;
    std_wstr name;
};

// 下载图片信息
class ImageInfo {
public:
    ImageInfo(ImageType type, std_cwstr_ref path)
        : image_type(type)
        , save_path(path) {
    }

    ImageInfo(ImageType type, std_wstr_r_ref path)
        : image_type(type)
        , save_path(std::move(path)) {
    }

    std_wstr save_path;
    ImageType image_type;
};