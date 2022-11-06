#include "system_utils.h"
#include <windows.h>
#include <shlobj.h>
#include <TlHelp32.h>

namespace system_utils {

    BOOL CopyToClipboard(const wchar_t* source)
    {
        if (source == nullptr) {
            return FALSE;
        }
        int length = wcslen(source);
        if (length == 0) {
            return FALSE;
        }
        if (!OpenClipboard(NULL)) {
            DWORD dwError = GetLastError();
            return FALSE;//打开剪切板失败
        }
        EmptyClipboard();
        SIZE_T nSize = length + 1;
        HGLOBAL hClip = GlobalAlloc(GMEM_DDESHARE, nSize * sizeof(wchar_t));
        PTSTR pszBuf = (PTSTR)GlobalLock(hClip);
        wcscpy_s(pszBuf, nSize, source);
        GlobalUnlock(hClip);
        SetClipboardData(CF_UNICODETEXT, hClip);
        CloseClipboard();
        return TRUE;
    }

    BOOL GetExePathW(wchar_t* path, uint32_t size)
    {
        if (path == nullptr || size < 64) {
            return FALSE;
        }
        ::GetModuleFileName(NULL, path, size);
        int length = wcslen(path);
        for (int i = 0; i < length; ++i) {
            if (path[length - 1 - i] == '\\') {
                break;
            }
            path[length - 1 - i] = '\0';
        }
        return TRUE;
    }

    BOOL GetExePathA(char* path, uint32_t size)
    {
        if (path == nullptr || size < 64) {
            return FALSE;
        }
        ::GetModuleFileNameA(NULL, path, size);
        int length = strlen(path);
        for (int i = 0; i < length; ++i) {
            if (path[length - 1 - i] == '\\') {
                break;
            }
            path[length - 1 - i] = '\0';
        }
        return TRUE;
    }

    uint32_t GetPidByName(const wchar_t* lpExeName, BOOL bIncludeSelf)
    {
        HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hSnapshot) {
            return 0;
        }
        PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
        BOOL bFlag = ::Process32First(hSnapshot, &pe);
        DWORD dwSelfID = ::GetCurrentProcessId();
        while (bFlag) {
            if (_wcsicmp(lpExeName, pe.szExeFile) == 0) {
                if (bIncludeSelf || (dwSelfID != pe.th32ProcessID)) {
                    ::CloseHandle(hSnapshot);
                    return pe.th32ProcessID;
                }
            }
            bFlag = ::Process32Next(hSnapshot, &pe);
        }
        ::CloseHandle(hSnapshot);
        return 0;
    }

    std::string GetAppTempPathA(const char* app_name)
    {
        char path[MAX_PATH + 1] = { 0 };
        ::GetTempPathA(MAX_PATH, path);
        strcat_s(path, app_name);
        CreateDirectoryA(path, NULL);
        strcat_s(path, "\\");
        return std::string(path);
    }

    std::wstring GetAppTempPathW(const wchar_t* app_name)
    {
        wchar_t path[MAX_PATH + 1] = { 0 };
        ::GetTempPathW(MAX_PATH, path);
        wcscat_s(path, app_name);
        CreateDirectoryW(path, NULL);
        wcscat_s(path, L"\\");
        return std::wstring(path);
    }

    void ActiveWindow(HWND hWnd)
    {
        ::ShowWindow(hWnd, SW_SHOWNORMAL);
        ::SetActiveWindow(hWnd);
        ::SetFocus(hWnd);
        ::SetForegroundWindow(hWnd);

        ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    BOOL GetFileContentA(const char* file, OUT std::string& content)
    {
        BOOL result = FALSE;
        HANDLE hFile = ::CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL, 
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            return result;
        }
        LARGE_INTEGER largeSize = { 0 };
        ::GetFileSizeEx(hFile, &largeSize);
        DWORD dwSize = (DWORD)largeSize.QuadPart;
        unsigned char* buffer = (unsigned char*)malloc(dwSize);
        if (buffer == NULL) {
            return result;
        }
        DWORD dwReadSize = 0;
        ::ReadFile(hFile, buffer, dwSize, &dwReadSize, NULL);
        if (dwSize == dwReadSize) {
            content.assign((char*)buffer, dwSize);
            result = TRUE;
        }
        free(buffer);
        ::CloseHandle(hFile);
        return result;
    }

    BOOL GetFileContentW(const wchar_t* file, OUT std::string& content)
    {
        BOOL result = FALSE;
        HANDLE hFile = ::CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            return result;
        }
        LARGE_INTEGER largeSize = { 0 };
        ::GetFileSizeEx(hFile, &largeSize);
        DWORD dwSize = (DWORD)largeSize.QuadPart;
        unsigned char* buffer = (unsigned char*)malloc(dwSize);
        if (buffer == NULL) {
            return result;
        }
        DWORD dwReadSize = 0;
        ::ReadFile(hFile, buffer, dwSize, &dwReadSize, NULL);
        if (dwSize == dwReadSize) {
            content.assign((char*)buffer, dwSize);
            result = TRUE;
        }
        free(buffer);
        ::CloseHandle(hFile);
        return result;
    }

    BOOL StretchImage(unsigned char* src, int width, int height, 
        int destWidth, int destHeight, unsigned char** ppdestBits)
    {
        byte* buffer = (byte*)malloc(destWidth * destHeight * 4);
        if (buffer == NULL) {
            return FALSE;
        }
        double horFactor = double(width) / destWidth;  //水平缩放因子
        double verFactor = double(height) / destHeight; //垂直缩放因子

        double w0, w1, w2, w3; // weight
        int x1, y1, x2, y2; // (x1, y1), (x2, y2)
        double fx1, fx2, fy1, fy2;

        for (int i = 0; i < destHeight; i++)
        {
            double x0 = i * verFactor;
            x1 = int(i * verFactor);
            x2 = x1 + 1;

            fx1 = x2 - x0;
            fx2 = x0 - x1;
            for (int j = 0; j < destWidth; j++)
            {
                double y0 = j * horFactor;
                y1 = int(j * horFactor);
                y2 = y1 + 1;

                fy1 = y2 - y0;
                fy2 = y0 - y1;

                // 计算权值
                w0 = fx1 * fy1;
                w1 = fx1 * fy2;
                w2 = fx2 * fy1;
                w3 = fx2 * fy2;

                int dstOffset = (i * destWidth + j) * 4;
                int srcOffset1 = (x1 * width + y1) * 4;
                int srcOffset2 = (x2 * width + y1) * 4;

                buffer[dstOffset + 0] = w0 * src[srcOffset1] + w1 * src[srcOffset1 + 4] + w2 * src[srcOffset2] + w3 * src[srcOffset2 + 4];     //B
                buffer[dstOffset + 1] = w0 * src[srcOffset1 + 1] + w1 * src[srcOffset1 + 5] + w2 * src[srcOffset2 + 1] + w3 * src[srcOffset2 + 5]; //G
                buffer[dstOffset + 2] = w0 * src[srcOffset1 + 2] + w1 * src[srcOffset1 + 6] + w2 * src[srcOffset2 + 2] + w3 * src[srcOffset2 + 6]; //R
                buffer[dstOffset + 3] = 0xff;
            }
        }
        *ppdestBits = buffer;
        return TRUE;
    }

}// namespace system_utils