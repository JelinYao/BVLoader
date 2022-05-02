#pragma once
#include "common_define.h"

// 解析视频页url
bool ParseBiliUrl(std_cstr_ref url, OUT std_str_ref detailUrl, OUT VideoType& type);

// 生成视频播放页信息url
std_str BuildPlayerUrl(VideoType type, __int64 aid, __int64 cid, int qn = 0);
