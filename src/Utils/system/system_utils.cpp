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
            return FALSE;//´ò¿ª¼ôÇÐ°åÊ§°Ü
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
            return NULL;
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

}// namespace system_utils