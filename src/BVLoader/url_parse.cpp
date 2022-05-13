#include "pch.h"
#include "url_parse.h"
#include "api_define.h"

#include <regex>
/**************************************************************************************
* C++11正则使用：https://zhuanlan.zhihu.com/p/137747912
* 正则?:用法：https://blog.csdn.net/u012028371/article/details/78968693
* 正则语法：https://www.runoob.com/regexp/regexp-syntax.html
*/

// UGC regex
constexpr const char* kUgcRegexString = "(?:BV|bv)([a-zA-Z0-9]+)";

bool ParseBiliUrl(std_cstr_ref url, OUT std_str_ref detailUrl, OUT VideoType& type)
{
    std::smatch m;
    if (std::regex_search(url, m, std::regex(kUgcRegexString))) {
        if (m.size() != 2) {
            return false;
        }
        // smatch 的第一个元素匹配整个字符串
        // smatch 的第二个元素匹配括号表达式
        detailUrl.assign(kUgcDetailApi);
        detailUrl.append(m[1].str());
        type = VideoType::VIDEO_UGC;
        return true;
    }
    return false;
}

std_str BuildPlayerUrl(VideoType type, __int64 aid, __int64 cid, int qn/* = 0*/)
{
    std_str url;
    switch (type)
    {
    case VideoType::VIDEO_UNKNOWN:
        break;
    case VideoType::VIDEO_UGC: {
        char buffer[256] = { 0 };
        sprintf_s(buffer, kUgcPlayerApi, aid, cid, qn);
        url.assign(buffer);
        break;
    }
    default:
        break;
    }
    return url;
}
