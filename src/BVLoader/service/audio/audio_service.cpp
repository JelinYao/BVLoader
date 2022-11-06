#include "pch.h"
#include "audio_service.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#pragma comment(lib, "avformat")
#pragma comment(lib, "avcodec")
#pragma comment(lib, "avutil")
#pragma comment(lib, "swresample")
#pragma comment(lib, "swscale")
#endif
#include "lame/lame.h"
#include "soft_define.h"
#include "audio_service_delegate.h"
#include <libjpegturbo/turbojpeg.h>

// 1 second of 48khz 32bit audio
static constexpr int kMaxAudioFrameSize = 192000;
AudioService::AudioService()
{
}

AudioService::~AudioService()
{
}

bool AudioService::Init()
{
    return true;
}

void AudioService::Exit()
{

}

void AudioService::AddDelegate(IAudioServiceDelegate* delegate, void* param)
{
    assert(delegate);
    delegate_ = delegate;
    param_ = param;
}

void AudioService::Decode(UINT_PTR task_id, std_cwstr_ref video_path, 
    std_cwstr_ref mp3_path, std_cwstr_ref img_path)
{
    DecodeErrorCode code = DecodeErrorCode::ERROR_SUCCESS;
    tjhandle decodeHandle = NULL;
    unsigned char* decodeBuffer = NULL;
    unsigned char* stretchBuffer = NULL;
    unsigned char* encodeBuffer = NULL;
    unsigned long encodeSize = 0;
    while (true) {
        // video to pcm
        auto ascii_video_path = string_utils::UToA(video_path);
        auto name = string_utils::GetNameByPathA(ascii_video_path, false);
        if (name.empty()) {
            LOG(ERROR) << "VideoToMp3 获取文件名失败, path: " << ascii_video_path;
            code = DecodeErrorCode::ERROR_INIT;
            break;
        }
        auto pcm_path = system_utils::GetAppTempPathA(kAppNameAscii) + name;
        pcm_path.append(".pcm");
        int samples_rate = 0;
        if (MpegExportPcm(ascii_video_path.c_str(), pcm_path.c_str(), &samples_rate) != 0) {
            LOG(ERROR) << "VideoToMp3 MpegExportPcm failed";
            code = DecodeErrorCode::ERROR_DECODE_VIDEO;
            break;
        }
        // 压缩图片
        while (true) {
            if (img_path.empty()) {
                break;
            }
            std_str content;
            if (!system_utils::GetFileContentW(img_path.c_str(), content)) {
                break;
            }
            decodeHandle = tjInitDecompress();
            int width = 0, height = 0, subsample = 0, colorspace = 0;
            int result = tjDecompressHeader3(decodeHandle, (const unsigned char*)content.c_str(), content.size(),
                &width, &height, &subsample, &colorspace);
            if (result < 0) {
                break;
            }
            if (width <= 500 && height <= 600) {
                break;
            }
            // BGRA  jpg解码
            decodeBuffer = (unsigned char*)malloc(width * height * 4);
            result = tjDecompress2(decodeHandle, (const unsigned char*)content.c_str(), content.size(),
                decodeBuffer, width, 0, height, TJPF_BGRA, TJFLAG_ACCURATEDCT);
            if (result != 0) {
                break;
            }
            // 压缩图片尺寸
            int destWidth = 500;
            int destHeight = destWidth * height / width;
            if (!system_utils::StretchImage(decodeBuffer, width, height,
                destWidth, destHeight, &stretchBuffer)) {
                break;
            }
            // jpg编码
            result = EncodeJpeg(stretchBuffer, destWidth, destHeight, &encodeBuffer, &encodeSize);
            break;
        }
        if (decodeHandle) {
            tjDestroy(decodeHandle);
        }
        if (decodeBuffer) {
            free(decodeBuffer);
        }
        if (stretchBuffer) {
            free(stretchBuffer);
        }
        // pcm to mp3
        auto ascii_mp3_path = string_utils::UToA(mp3_path);
        if (PcmToMp3(pcm_path.c_str(), ascii_mp3_path.c_str(), samples_rate, 2, 
            128, (const char*)encodeBuffer, encodeSize) != 0) {
            LOG(ERROR) << "VideoToMp3 PcmToMp3 failed";
            code = DecodeErrorCode::ERROR_ENCODE_MP3;
            break;
        }
        break;
    }
    if (encodeBuffer) {
        free(encodeBuffer);
    }
    if (delegate_) {
        delegate_->OnDecodeComplete(task_id, code, param_);
    }
}

int AudioService::MpegExportPcm(const char* video_path, const char* pcm_file, int* samples_rate)
{
    int result = 0;
    av_register_all();
    AVFormatContext* format_context = NULL;
    if (0 != avformat_open_input(&format_context, video_path, NULL, NULL)) {
        LOG(ERROR) << "MpegExportPcm avformat_open_input failed";
        return -1;
    }
    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0)
    {
        LOG(ERROR) << "MpegExportPcm avformat_find_stream_info failed, code: " << result;
        avformat_close_input(&format_context);
        return -1;
    }
    //查找第一个音频流
    int audio_index = -1;
    for (unsigned int i = 0; i < format_context->nb_streams; ++i) {
        if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_index = i;
            break;
        }
    }
    if (audio_index == -1)
    {
        LOG(ERROR) << "MpegExportPcm 查找音频流失败";
        avformat_close_input(&format_context);
        return -1;
    }
    AVCodecContext* codes_context = format_context->streams[audio_index]->codec;
    // 找到解码器
    AVCodec* pCodec = avcodec_find_decoder(codes_context->codec_id);
    if (NULL == pCodec) {
        LOG(ERROR) << "MpegExportPcm 查找解码器失败";
        avformat_close_input(&format_context);
        return -1;
    }
    result = avcodec_open2(codes_context, pCodec, NULL);
    if (result < 0) {
        LOG(ERROR) << "MpegExportPcm 打开解码器失败";
        avformat_close_input(&format_context);
        return -1;
    }
    // 音频相关数据结构
    int out_nb_samples = codes_context->frame_size;
    AVSampleFormat format = AV_SAMPLE_FMT_S16;
    int out_rate = codes_context->sample_rate;// 44100;
    *samples_rate = out_rate;
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, format, 1);
    FILE* fp = NULL;
    fopen_s(&fp, pcm_file, "ab+");
    if (fp == NULL) {
        LOG(ERROR) << "MpegExportPcm 打开文件：" << pcm_file << "失败，系统错误码：" << GetLastError();
        avformat_close_input(&format_context);
        return -1;
    }
    uint8_t* out_buffer = (uint8_t*)av_malloc(kMaxAudioFrameSize);
    // FIX:Some Codec's Context Information is missing  
    int64_t in_channel_layout = av_get_default_channel_layout(codes_context->channels);
    SwrContext* swrCtx = swr_alloc();
    swrCtx = swr_alloc_set_opts(swrCtx, out_channel_layout, format, out_rate, in_channel_layout, codes_context->sample_fmt, codes_context->sample_rate, 0, NULL);
    swr_init(swrCtx);
    AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket));
    av_init_packet(packet);
    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == audio_index) {
            AVFrame* pFrame = av_frame_alloc();
            int finish = 0;
            result = avcodec_decode_audio4(codes_context, pFrame, &finish, packet);
            if (finish) {
                result = swr_convert(swrCtx, &out_buffer, kMaxAudioFrameSize, (const uint8_t**)pFrame->data, pFrame->nb_samples);
                int data_size = pFrame->channels * result * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//获取实际传输的字节数
                fwrite(out_buffer, data_size, 1, fp);
            }
            av_frame_free(&pFrame);
        }
    }
    fclose(fp);
    swr_free(&swrCtx);
    av_packet_unref(packet);
    free(packet);
    av_free(out_buffer);
    avformat_close_input(&format_context);
    return 0;
}

int AudioService::PcmToMp3(const char* pcm_file, const char* mp3_file, int samples_rate, 
    int channels, int brate, const char* imgBuffer, size_t imgSize)
{
    FILE* fp_pcm = NULL;
    fopen_s(&fp_pcm, pcm_file, "rb");
    if (NULL == fp_pcm) {
        LOG(ERROR) << "PcmToMp3 打开文件：" << pcm_file << "失败，系统错误码：" << GetLastError();
        return -1;
    }
    FILE* fp_mp3 = NULL;
    fopen_s(&fp_mp3, mp3_file, "wb+");
    if (NULL == fp_mp3) {
        LOG(ERROR) << "PcmToMp3 打开文件：" << mp3_file << "失败，系统错误码：" << GetLastError();
        fclose(fp_pcm);
        return -1;
    }
    // 初始化编码信息
    lame_global_flags* lame = lame_init();
    lame_set_out_samplerate(lame, samples_rate);
    lame_set_in_samplerate(lame, samples_rate); // 设置输出音频的采样频率(48000Hz)
    lame_set_num_channels(lame, channels); // 设置输出音频的声道数
    lame_set_brate(lame, brate); // 设置输出音频的比特率
    // 音频大小 = (比特率*1024/8.0)*(时长:秒)
    lame_set_quality(lame, 2);// quality=0..9.  0=best (very slow).  9=worst.
    id3tag_init(lame);
    id3tag_add_v2(lame);
    id3tag_space_v1(lame);
    id3tag_pad_v2(lame);
    // 设置歌曲信息
    if (imgBuffer && imgSize > 0) {
        id3tag_set_albumart(lame, imgBuffer, imgSize);
    }
    // 获取MP3文件头大小
    // size_t id3_size = lame_get_id3v2_tag(lame, NULL, 0);
    size_t tag_size = 1024 * 1024;
    unsigned char* tag_buffer = (unsigned char*)malloc(tag_size);
    size_t id3_size = lame_get_id3v2_tag(lame, tag_buffer, tag_size);
    lame_init_params(lame);
    const int buffer_size = 8192, mp3_buffer_size = buffer_size + id3_size * 2;
    unsigned char* mp3_buffer = (unsigned char*)malloc(mp3_buffer_size);
    short int* pcm_buffer = (short int*)malloc(buffer_size * channels * sizeof(short int));
    while (true) {
        size_t read_bytes = fread(pcm_buffer, sizeof(short int) * channels, buffer_size, fp_pcm);
        if (read_bytes < 1) {
            break;
        }
        // lame编码pcm
        int write_bytes = lame_encode_buffer_interleaved(lame, pcm_buffer, read_bytes, mp3_buffer, mp3_buffer_size);
        if (write_bytes > 0) {
            fwrite(mp3_buffer, 1, write_bytes, fp_mp3);
        }
        // 最后一次，全部编码完
        if (read_bytes < buffer_size) {
            write_bytes = lame_encode_flush(lame, mp3_buffer, mp3_buffer_size);
            if (write_bytes > 0) {
                fwrite(mp3_buffer, 1, write_bytes, fp_mp3);
            }
            break;
        }
    }
    free(tag_buffer);
    free(pcm_buffer);
    free(mp3_buffer);
    fclose(fp_mp3);
    fclose(fp_pcm);
    lame_close(lame);
    return 0;
}

int AudioService::EncodeJpeg(unsigned char* src, int width, int height,
    OUT unsigned char** ppJpegBuf, OUT unsigned long* jpegSize)
{
    tjhandle handle = tjInitCompress();
    int pixelFormat = TJPF_BGRA;
    int result = tjCompress2(handle, src, width, 0, height, pixelFormat,
        ppJpegBuf, jpegSize, TJSAMP_420, 60, TJFLAG_ACCURATEDCT);
    if (result < 0) {
        char* error = tjGetErrorStr2(handle);
        *ppJpegBuf = NULL;
        *jpegSize = 0;
        tjDestroy(handle);
        return -1;
    }
    tjDestroy(handle);
    return 0;
}