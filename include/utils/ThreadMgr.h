#pragma once
#include <windows.h>
#include <list>
#include <string>

namespace httplib
{
    class CThreadMgr;
    class CThread
    {
    public:
        CThread();
        virtual ~CThread();

        void SetThreadName(const char* threadName);

    public:
        BOOL CreateThread(const char* lpszThreadName, void* argument);
        void StopThread();

        BOOL SuspendThread();
        BOOL ResumeThread();

        BOOL IsNeedExit();
        BOOL IsHasExited();
        void SetHasExited();

        unsigned int GetThreadId();
        HANDLE GetThreadHandle();

        void* GetArgList();
        void SetThreadManager(CThreadMgr* pThreadManager);
        CThreadMgr* GetThreadManager();

    protected:
        unsigned int m_dwThreadId;
        HANDLE m_hThread;
        BOOL m_bNeedExit;
        BOOL m_bHasExited;

    protected:
        void* m_pArgList; //传递参数临时变量
        CThreadMgr* m_pThreadManager;
        std::string m_strThreadName;

    protected:
        static unsigned __stdcall ThreadProc(void* argument);

    protected://Need Override
        virtual int Run(void* argument) = 0;
    };

    class CThreadMgr
    {
    public:
        CThreadMgr(void);
        virtual ~CThreadMgr(void);

        BOOL AddThread(CThread* p);
        BOOL DeleteThread(CThread* p);

        BOOL ExitAllThread();

        void LockThreadList();
        void UnlockThreadList();
    protected:
        std::list<CThread* > m_threadList;
        CRITICAL_SECTION m_threadListCritialSection;
    };

}