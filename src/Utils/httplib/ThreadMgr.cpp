#include "ThreadMgr.h"
#include "SE.h"
#include <process.h>

namespace httplib
{
    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)


    CThread::CThread()
    {
        m_bNeedExit = FALSE;
        m_bHasExited = FALSE;

        m_dwThreadId = 0;
        m_hThread = INVALID_HANDLE_VALUE;

        m_pArgList = NULL;
        m_pThreadManager = NULL;

        m_strThreadName = "";
    }

    CThread::~CThread()
    {
        this->StopThread();

        if (this->m_hThread != INVALID_HANDLE_VALUE)
        {
            int i = 0;
            for (; i < 5; i++)
            {
                DWORD dwRet = ::WaitForSingleObject(this->m_hThread, 1000);
                if (dwRet == WAIT_TIMEOUT)
                {
                    continue;
                }
                else
                {
                    CloseHandle(this->m_hThread);
                    this->m_hThread = INVALID_HANDLE_VALUE;
                    break;
                }
            }
        }

        if (this->m_hThread != INVALID_HANDLE_VALUE)
        {
            char msg[128] = { 0 };
            sprintf_s(msg, "线程 %s 退出超时（5s）,请查看代码！！", this->m_strThreadName.c_str());
#ifdef _DEBUG
            ::MessageBoxA(NULL, msg, "警告!!!", MB_OK | MB_ICONWARNING);
#endif //_DEBUG
            OutputDebugStringA(msg);
        }
    }

    void CThread::SetThreadName(const char* lpszThreadName)
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = lpszThreadName;
        info.dwThreadID = m_dwThreadId;
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        m_strThreadName = lpszThreadName;
    }

    BOOL CThread::CreateThread(const char* lpszThreadName, void* argument)
    {
        if (m_bHasExited == TRUE)
            return FALSE;

        m_pArgList = argument;
        m_hThread = (HANDLE)::_beginthreadex(NULL, 0, CThread::ThreadProc, this, CREATE_SUSPENDED, &m_dwThreadId);
        if (m_dwThreadId == 0)
        {
            return FALSE;
        }

        if (lpszThreadName != NULL)
        {
            this->SetThreadName(lpszThreadName);
        }

        return TRUE;
    }


    void CThread::StopThread()
    {
        m_bNeedExit = TRUE;
    }

    BOOL CThread::SuspendThread()
    {
        ::SuspendThread(m_hThread);

        return TRUE;
    }

    BOOL CThread::ResumeThread()
    {
        ::ResumeThread(m_hThread);

        return TRUE;
    }

    BOOL CThread::IsNeedExit()
    {
        return m_bNeedExit;
    }

    BOOL CThread::IsHasExited()
    {
        return m_bHasExited;
    }

    void CThread::SetHasExited()
    {
        m_bHasExited = TRUE;
    }

    unsigned int CThread::GetThreadId()
    {
        return m_dwThreadId;
    }

    HANDLE CThread::GetThreadHandle()
    {
        return m_hThread;
    }

    void* CThread::GetArgList()
    {
        return m_pArgList;
    }

    void CThread::SetThreadManager(CThreadMgr* pThreadManager)
    {
        m_pThreadManager = pThreadManager;
    }

    CThreadMgr* CThread::GetThreadManager()
    {
        return m_pThreadManager;
    }

    unsigned __stdcall CThread::ThreadProc(void* argument)
    {
        unsigned int result = 0;
        if (CSE::IsEnableMapSEtoCE())
        {
            //如果主线程加载“加载结构化异常转C++异常”，那么每个线程启动的时候，也自动执行一次映射。
            //因为映射适合线程相关的
            CSE::MapSEtoCE();
        }

        CThread* pThis = (CThread*)argument;

        try
        {
            result = pThis->Run(pThis->GetArgList());
        }
        catch (CSE& SE)
        {
            ::MessageBoxA(NULL, "ThreadProc error!", "", MB_OK);
        }

        if (pThis->GetThreadManager() != NULL)
        {
            pThis->GetThreadManager()->DeleteThread(pThis);
        }

        pThis->SetHasExited();
        return result;
    }

    CThreadMgr::CThreadMgr(void)
    {
        ::InitializeCriticalSection(&m_threadListCritialSection);
    }

    CThreadMgr::~CThreadMgr(void)
    {
        ::DeleteCriticalSection(&m_threadListCritialSection);
    }



    BOOL CThreadMgr::AddThread(CThread* p)
    {
        this->LockThreadList();
        m_threadList.push_back(p);
        this->UnlockThreadList();

        p->SetThreadManager(this);

        return TRUE;
    }

    BOOL CThreadMgr::DeleteThread(CThread* p)
    {
        this->LockThreadList();

        std::list<CThread*>::iterator iter;
        for (iter = m_threadList.begin(); iter != m_threadList.end();)
        {
            if (*iter == p)
            {
                iter = m_threadList.erase(iter);
            }
            else
            {
                iter++;
            }
        }

        this->UnlockThreadList();

        return TRUE;
    }

    BOOL CThreadMgr::ExitAllThread()
    {
        this->LockThreadList();
        std::list<CThread*>::iterator iter;
        for (iter = m_threadList.begin(); iter != m_threadList.end();)
        {
            CThread* p = *iter;
            p->StopThread();
        }
        this->UnlockThreadList();

        int n_size = 0;
        for (int i = 0; i < 5; i++)
        {
            this->LockThreadList();
            n_size = m_threadList.size();
            this->LockThreadList();

            if (n_size != 0)
                ::Sleep(1000);
        }

        return TRUE;
    }

    void CThreadMgr::LockThreadList()
    {
        ::EnterCriticalSection(&m_threadListCritialSection);
    }

    void CThreadMgr::UnlockThreadList()
    {
        ::LeaveCriticalSection(&m_threadListCritialSection);
    }

}