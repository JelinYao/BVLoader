#pragma once
#include "common_define.h"

// ������Ƶҳurl
bool ParseBiliUrl(std_cstr_ref url, OUT std_str_ref detailUrl, OUT VideoType& type);

// ������Ƶ����ҳ��Ϣurl
std_str BuildPlayerUrl(VideoType type, __int64 aid, __int64 cid, int qn = 0);
