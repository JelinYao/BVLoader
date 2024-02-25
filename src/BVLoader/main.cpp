// BVLoader.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "Resource.h"
#include "soft_define.h"
#include "wnd_main.h"
#include "service_manager.h"

INITIALIZE_EASYLOGGINGPP

void InitEasyLog();
BOOL CheckInstance(HANDLE& hMutex);
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    InitEasyLog();
    LOG(INFO) << "Application start run, version: " << kAppVersion;
    HANDLE mutex = NULL;
    if (CheckInstance(mutex)) {
        LOG(INFO) << "Process is already running, exit";
        return 0;
    }

    CPaintManagerUI::SetInstance(hInstance);
#ifdef _DEBUG
    CDuiString wstrResoucePath = CPaintManagerUI::GetInstancePath();
    wstrResoucePath += L"skin";
    CPaintManagerUI::SetResourcePath(wstrResoucePath);
#else
    BYTE* pSkinBuffer = NULL;
    HRSRC hRes = ::FindResource(NULL, MAKEINTRESOURCE(IDR_DAT1), L"DAT");
    HGLOBAL hGlobal = ::LoadResource(NULL, hRes);
    DWORD dwSkinBufferSize = ::SizeofResource(NULL, hRes);
    BYTE* pResource = (BYTE*)::LockResource(hGlobal);
    CPaintManagerUI::SetResourceZip(pResource, dwSkinBufferSize);
#endif
    ServiceManager::Instance()->Init(hInstance);
    std::shared_ptr<WndMain> wnd = std::make_shared<WndMain>();
    wnd->Create(NULL);
    wnd->CenterWindow();
    wnd->ShowWindow();
    CPaintManagerUI::MessageLoop();
    CPaintManagerUI::Term();
    ServiceManager::Instance()->Exit();
    ::CloseHandle(mutex);
    LOG(INFO) << "Application exit";
#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}

void InitEasyLog()
{
    std::string format;
    format = "%datetime %level [A] %fbase:%line %msg ";
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(el::Level::Info,
        el::ConfigurationType::Format, format);
    defaultConf.set(el::Level::Debug,
        el::ConfigurationType::Format, format);
    defaultConf.set(el::Level::Error,
        el::ConfigurationType::Format, format);
    defaultConf.set(el::Level::Warning,
        el::ConfigurationType::Format, format);

    char log_path[MAX_PATH] = { 0 };
    system_utils::GetExePathA(log_path, MAX_PATH);
    // ::SHGetSpecialFolderPathA(NULL, log_path, CSIDL_APPDATA, TRUE);
    strcat_s(log_path, "log\\");
    SHCreateDirectoryExA(NULL, log_path, NULL);
    SYSTEMTIME stCur;
    ::GetLocalTime(&stCur);
    char current_date[128] = { 0 };
    sprintf_s(current_date, 128, "BVLoader_%04d-%02d-%02d.log", stCur.wYear,
        stCur.wMonth, stCur.wDay);
    strcat_s(log_path, current_date);
    std::string log_file(log_path);

    defaultConf.set(el::Level::Info,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Debug,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Error,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Warning,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Global,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Error,
        el::ConfigurationType::Filename, log_file);
    el::Loggers::reconfigureLogger("default", defaultConf);
}

BOOL CheckInstance(HANDLE& hMutex)
{
    hMutex = CreateMutex(NULL, TRUE, kAppInstanceMutex);
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWnd = ::FindWindow(kAppWindowClassName, kAppWindowTitle);
        if (hWnd) {
            CloseHandle(hMutex);
            return TRUE;
        }
        //清理残留进程
        DWORD dwPid = system_utils::GetPidByName(kAppExeName, FALSE);
        if (dwPid && dwPid != GetCurrentProcessId()) {
            HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
            if (hProcess) {
                ::TerminateProcess(hProcess, 0);
                ::CloseHandle(hProcess);
            }
        }
    }
    return FALSE;
}
