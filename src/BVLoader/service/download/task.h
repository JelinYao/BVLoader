#pragma once
#include <utils/HttpLib.h>

namespace download {
    // ÏÂÔØ×´Ì¬
    enum class DownloadStatus {
        STATUS_INIT = 0,
        STATUS_WAITTING,
        STATUS_LOADING,
        STATUS_PAUSE,
        STATUS_DECODE,
        STATUS_FAILED,
        STATUS_DOWNLOAD_SUCCESS,
        STATUS_FINISH,
    };

    class Task {
    public:
        Task() noexcept {
        }

        Task(const Task& t) noexcept {
            url = t.url;
            save_path = t.save_path;
            title = t.title;
            author = t.author;
            img = t.img;
            audio_path = t.audio_path;
            duration = t.duration;
            ctime = t.ctime;
            total_size = t.total_size;
            load_size = t.load_size;
            status = t.status;
            ltime = 0L;
            temp_size = 0.0F;
        }

        Task(Task&& t) noexcept {
            url = std::move(t.url);
            save_path = std::move(t.save_path);
            title = std::move(t.title);
            author = std::move(t.author);
            img = std::move(t.img);
            audio_path = std::move(t.audio_path);
            duration = t.duration;
            ctime = t.ctime;
            total_size = t.total_size;
            load_size = t.load_size;
            status = t.status;
            http = std::move(t.http);
            ltime = 0L;
            temp_size = 0.0F;
        }

        Task& operator=(const Task& t) {
            if (this != &t) {
                url = t.url;
                save_path = t.save_path;
                title = t.title;
                author = t.author;
                audio_path = t.audio_path;
                img = t.img;
                duration = t.duration;
                ctime = t.ctime;
                total_size = t.total_size;
                load_size = t.load_size;
                status = t.status;
                ltime = 0L;
                temp_size = 0.0F;
            }
            return *this;
        }

        Task& operator=(Task&& t) noexcept {
            if (this != &t) {
                url = std::move(t.url);
                save_path = std::move(t.save_path);
                title = std::move(t.title);
                author = std::move(t.author);
                img = std::move(t.img);
                audio_path = std::move(t.audio_path);
                duration = t.duration;
                ctime = t.ctime;
                total_size = t.total_size;
                load_size = t.load_size;
                status = t.status;
                http = std::move(t.http);
                ltime = 0L;
                temp_size = 0.0F;
            }
            return *this;
        }

        virtual ~Task() {
            if (http) {
                http->Stop();
                http = nullptr;
            }
        }

        void Clear() noexcept {
            url.clear();
            save_path.clear();
            title.clear();
            author.clear();
            img.clear();
            audio_path.clear();
            duration = 0;
            ctime = 0L;
            ltime = 0L;
            total_size = 0.0F;
            load_size = 0.0F;
            temp_size = 0.0F;
            status = DownloadStatus::STATUS_INIT;
        }

        std_str url;
        std_wstr save_path;
        std_wstr title;
        std_wstr author;
        std_wstr img;
        std_wstr audio_path;
        int duration = 0;
        __int64 ctime = 0L;
        __int64 ltime = 0L;
        double total_size = 0.0F;
        double load_size = 0.0F;
        double temp_size = 0.0F;
        std::shared_ptr<httplib::CHttpDownload> http;
        DownloadStatus status = DownloadStatus::STATUS_INIT;
    };
} // namespace download