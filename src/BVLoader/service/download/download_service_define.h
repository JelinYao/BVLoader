#pragma once

namespace download {

    static constexpr const int kBaseWindowMessage = WM_USER + 600;
    // enum window message
    enum MsgWindowMessage {
        WM_MSG_UPDATE_TASK_PROGRESS = kBaseWindowMessage,
        WM_MSG_UPDATE_TASK_STATUS,
    };

} // namespace download
