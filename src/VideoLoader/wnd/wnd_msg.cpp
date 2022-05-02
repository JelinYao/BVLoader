#include "pch.h"
#include "wnd_msg.h"
#include "soft_define.h"

WndMsg::WndMsg(MessageIconType icon, LPCWSTR info)
    : icon_type_(icon)
    , info_(info)
{
    m_dwStyle = UI_WNDSTYLE_DIALOG;
    m_bEscape = true;
}

WndMsg::~WndMsg()
{
}

LPCWSTR WndMsg::GetWndName() const
{
    return kWndMsgTitle;
}

void WndMsg::InitWindow()
{
    WndBase::InitWindow();
    auto ctrl = m_pm.FindControl(L"text_info");
    assert(ctrl);
    ctrl->SetText(info_);
    ctrl = m_pm.FindControl(L"ctrl_icon");
    assert(ctrl);
    LPCWSTR img_path = nullptr;
    switch (icon_type_)
    {
    case MessageIconType::ICON_ERROR:
        img_path = L"msg\\error.png";
        break;
    case MessageIconType::ICON_ASK:
        img_path = L"msg\\ask.png";
        break;
    case MessageIconType::ICON_OK:
        img_path = L"msg\\ok.png";
        break;
    default:
        break;
    }
    assert(img_path);
    ctrl->SetBkImage(img_path);
}

void WndMsg::OnClick(TNotifyUI& msg)
{
    if (msg.pSender->GetName().Compare(L"btn_close") == 0) {
        Close(IDCLOSE);
    }
    else if (msg.pSender->GetName().Compare(L"btn_ok") == 0) {
        Close(IDOK);
    }
    else if (msg.pSender->GetName().Compare(L"btn_cancel") == 0) {
        Close(IDCANCEL);
    }
}

