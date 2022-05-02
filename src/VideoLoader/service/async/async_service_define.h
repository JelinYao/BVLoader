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