#pragma once
#include <windows.h>

static constexpr unsigned int kBaseWindowMessage = WM_USER + 1000;

// wnd parse message
enum  {
    WM_PARSEWND_ASYNC_SUCCESS = kBaseWindowMessage + 1,
    WM_PARSEWND_ASYNC_ERROR,
};

enum class MessageIconType {
    ICON_ERROR = 0,
    ICON_ASK,
    ICON_OK,
};

enum {
    WM_MAINWND_MSGBOX = kBaseWindowMessage + 1,
    WM_MAINWND_DECODE,
    WM_MAINWND_NOTIFY_STATUS,
};

enum MsgWparam {
    WPARAM_DELETE_LIST_ITEM = 0,
};