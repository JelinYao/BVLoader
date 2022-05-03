#pragma once
#include "wnd_base.h"
#include "message_define.h"

class WndMsg
    : public WndBase
{
public:
    WndMsg(MessageIconType icon, LPCWSTR info);
    ~WndMsg();

protected:
    LPCWSTR GetWndName()const override;
    LPCWSTR GetXmlPath()const override {
        return L"wnd_msg.xml";
    }

    void InitWindow() override;
    void OnClick(TNotifyUI& msg) override;

private:
    MessageIconType icon_type_ = MessageIconType::ICON_OK;
    CDuiString info_;
};