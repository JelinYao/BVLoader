#include "SE.h"

namespace httplib
{
    bool CSE::IsEnableMapSEtoCE()
    {
        return m_bEnableMapSEtoCE;
    }

    void CSE::MapSEtoCE()
    {
        _set_se_translator(TranslateSEtoCE);
        m_bEnableMapSEtoCE = true;
    }

    CSE::CSE(PEXCEPTION_POINTERS pep)
    {

        m_er = *pep->ExceptionRecord;
        m_context = *pep->ContextRecord;

    }

    void _cdecl CSE::TranslateSEtoCE(UINT dwEC, PEXCEPTION_POINTERS pep)
    {
        throw CSE(pep);
    }

    bool CSE::m_bEnableMapSEtoCE = false;

}