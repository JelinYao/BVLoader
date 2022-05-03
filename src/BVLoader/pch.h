#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <ShlObj.h>
#include <shlwapi.h>

// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <string>
#include <unordered_map>
#include <restclient/restclient.h>
#include <restclient/connection.h>
#include <json/json-forwards.h>
#include <json/json.h>
#include <utils/easylogging++.h>
#include <utils/system_utils.h>
#include <Utils/string_utils.h>
#include <UIlib.h>
using namespace duilib;
#pragma comment(lib, "Utils.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "DuiLib.lib")

using std_str = std::string;
using std_str_ref = std::string&;
using std_str_r_ref = std::string&&;
using std_cstr_ref = const std::string&;

using std_wstr = std::wstring;
using std_wstr_ref = std::wstring&;
using std_wstr_r_ref = std::wstring&&;
using std_cwstr_ref = const std::wstring&;
