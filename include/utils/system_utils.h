#pragma once
#include <stdint.h>
#include <windows.h>
#include <string>

namespace system_utils {

    BOOL CopyToClipboard(const wchar_t* source);

    BOOL GetExePathW(wchar_t* path, uint32_t size);

    BOOL GetExePathA(char* path, uint32_t size);

    uint32_t GetPidByName(const wchar_t* lpExeName, BOOL bIncludeSelf);

    std::string GetAppTempPathA(const char* app_name);

    std::wstring GetAppTempPathW(const wchar_t* app_name);

}// namespace system_utils

