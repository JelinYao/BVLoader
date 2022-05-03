#pragma once
#include <list>
#include <memory>
#include <utils/HttpLib.h>

namespace download {
    static constexpr size_t kMaxHttpCacheCount = 10;
    class HttpCache {
    public:
        HttpCache() {

        }

        ~HttpCache() {
            cache_list_.clear();
        }

        void Add(std::shared_ptr<httplib::CHttpDownload>&& http) {
            cache_list_.emplace_back(std::move(http));
            if (cache_list_.size() >= kMaxHttpCacheCount) {
                Clear();
            }
        }

        void Exit() {
            for (auto& http : cache_list_) {
                if (!http->IsHasExited()) {
                    http->Stop();
                }
            }
            cache_list_.clear();
        }

    protected:
        void Clear() {
            for (auto iter = cache_list_.begin(); iter != cache_list_.end(); ) {
                if ((*iter)->IsHasExited()) {
                    iter = cache_list_.erase(iter);
                    continue;
                }
                ++iter;
            }
        }

    private:
        std::list<std::shared_ptr<httplib::CHttpDownload>> cache_list_;
    };
}// namespace download