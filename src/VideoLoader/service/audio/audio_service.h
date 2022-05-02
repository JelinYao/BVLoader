#pragma once
#include "service.h"

class IAudioServiceDelegate;
class AudioService 
    : public IAudioService {
public:
    AudioService();
    ~AudioService();

    bool Init() override;
    void Exit() override;
    void AddDelegate(IAudioServiceDelegate* delegate, void* param) override;
    void Decode(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path) override;

protected:
    // ffmpeg decode
    int MpegExportPcm(const char* video_path, const char* pcm_file, int* samples_rate);
    // libmp3lame decode
    // samples_per_secondes������Ƶ�ʣ�Ĭ��44100Hz
    // channels����������Ĭ��2
    // brate�������ʣ�Ĭ��128
    int PcmToMp3(const char* pcm_file, const char* mp3_file, int samples_rate, int channels, int brate);

private:
    void* param_ = nullptr;
    IAudioServiceDelegate* delegate_ = nullptr;
};