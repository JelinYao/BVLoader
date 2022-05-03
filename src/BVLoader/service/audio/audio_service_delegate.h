#pragma once

#ifdef ERROR_SUCCESS
#undef ERROR_SUCCESS
#endif
enum class DecodeErrorCode {
    ERROR_SUCCESS = 0,
    ERROR_INIT,
    ERROR_DECODE_VIDEO,
    ERROR_ENCODE_MP3,

};
class IAudioServiceDelegate {
public:
    virtual void OnDecodeComplete(UINT_PTR task_id, DecodeErrorCode code, void* data) = 0;
};