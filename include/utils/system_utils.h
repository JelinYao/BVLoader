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

    void ActiveWindow(HWND hWnd);

    BOOL GetFileContentA(const char* file, OUT std::string& content);

    BOOL GetFileContentW(const wchar_t* file, OUT std::string& content);

    // 双线性插值缩放图片
    int StretchImage(unsigned char* src, int width, int height, 
        int destWidth, int destHeight, unsigned char** ppdestBits);
}// namespace system_utils

