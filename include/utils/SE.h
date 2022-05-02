#pragma once
#include <windows.h>
#include <eh.h>


namespace httplib
{
    class CSE
    {
    public:
        static bool IsEnableMapSEtoCE();
        static void MapSEtoCE();//Call this function for each thread.

        DWORD GetExpCode() { return m_er.ExceptionCode; }

    private:
        CSE(PEXCEPTION_POINTERS pep);

        static void _cdecl TranslateSEtoCE(UINT dwEC, PEXCEPTION_POINTERS pep);

    private:
        EXCEPTION_RECORD m_er; // CPU independent exception information
        CONTEXT m_context;     // CPU dependent exception information

        static bool m_bEnableMapSEtoCE;
    };

}