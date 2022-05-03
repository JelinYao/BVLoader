#include "pch.h"
#include "async_service.h"
#include "async_task.h"
#include "async_service_delegate.h"
#include "url_parse.h"
#include <utils/HttpLib.h>
#include "soft_define.h"
#include "service_manager.h"

inline void InitHttp(RestClient::Connection& http)
{
    http.SetUserAgent(kDefaultUserAgent);
    http.SetConnectTimeout(kDefaultHttpTimeOut);
    http.SetTimeout(kDefaultHttpTimeOut);
    http.AppendHeader("Referer", kDeafultReferer);
}

AsyncService::AsyncService()
{
}

AsyncService::~AsyncService()
{
}

bool AsyncService::Init()
{
    async_thread_ = std::make_unique<std::thread>(ThreadProc, this);
    if (!async_thread_) {
        LOG(ERROR) << "Init make_unique failed";
        return false;
    }
    return true;
}

void AsyncService::Exit()
{
    need_exit_ = true;
    task_event_.notify_one();
    if (async_thread_->joinable()) {
        async_thread_->join();
    }
}

void AsyncService::AddDelegate(IAsyncServiceDelegate* delegate, void* param)
{
   delegate_ = delegate;
}

void AsyncService::AddHttpTask(AsyncTaskType task_type, VideoType video_type, std_cstr_ref url)
{
    if (url.empty()) {
        LOG(ERROR) << "AddTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<HttpTask>(task_type, video_type, url));
    task_event_.notify_one();
}

void AsyncService::AddDecodeTask(UINT_PTR task_id, std_cwstr_ref video_path, std_cwstr_ref mp3_path)
{
    if (video_path.empty() || mp3_path.empty()) {
        LOG(ERROR) << "AddTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<DecodeTask>(task_id, video_path, mp3_path));
    task_event_.notify_one();
}

void AsyncService::ThreadProc(void* param)
{
    RestClient::init();
    AsyncService* service = reinterpret_cast<AsyncService*>(param);
    if (service) {
        service->DoAsyncWork();
    }
    RestClient::disable();
}

void AsyncService::DoAsyncWork()
{
    while (!need_exit_) {
        std::unique_lock<std::mutex> lock(task_mutex_);
        task_event_.wait(lock, [this] {
            return need_exit_ || !task_list_.empty();
            });
        if (need_exit_) {
            break;
        }
        auto iter = task_list_.begin();
        std::shared_ptr<IAsyncTask> task = std::move(*iter);
        task_list_.pop_front();
        lock.unlock();
        switch (task->task_type)
        {
        case AsyncTaskType::TASK_GET_INFO:
            OnGetVideoInfo(task);
            break;
        case AsyncTaskType::TASK_DOWNLOAD_COVER:
            OnDownloadCover(task);
            break;
        case AsyncTaskType::TASK_GET_PLAYER_URL:
        case AsyncTaskType::TASK_GET_SELECT_PLAYER_URL:
            OnGetPlayerUrl(task);
            break;
        case AsyncTaskType::TASK_DECODE_VIDEO:
            OnDecodeVideo(task);
            break;
        default:
            break;
        }
    }
}

void AsyncService::OnGetVideoInfo(const std::shared_ptr<IAsyncTask>& task)
{
    auto http_task = dynamic_pointer_cast<HttpTask>(task);
    if (http_task == nullptr) {
        LOG(ERROR) << "OnGetVideoInfo 转换AsyncTask对象失败";
        return;
    }
    auto response = DoHttpRequest(http_task);
    if(response.empty()) {
        return;
    }
    // 数据解析
    bool result = false;
    switch (http_task->video_type)
    {
    case VideoType::VIDEO_UGC: {
        result = (task->task_type == AsyncTaskType::TASK_GET_INFO) ?
            ParseUgcInfo(http_task, response) : ParseUgcPlayerUrl(http_task, response);
        break;
    }
    default:
        break;
    }
    if (!result && delegate_) {
        delegate_->OnAsyncComplete(task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
    }
}

void AsyncService::OnDownloadCover(const std::shared_ptr<IAsyncTask>& task)
{
    auto http_task = dynamic_pointer_cast<HttpTask>(task);
    if (http_task == nullptr) {
        LOG(ERROR) << "OnDownloadCover 转换AsyncTask对象失败";
        return;
    }
    httplib::CHttpDownload http(false);
    http.AddHeader("Referer", kDeafultReferer);
    auto temp_path = system_utils::GetAppTempPathW(kAppName);
    temp_path += string_utils::GetRandomText(10);
    temp_path.append(L".jpg");
    http.Initialize(http_task->url, temp_path);
    bool result = http.Start();
    if (delegate_) {
        std::wstring* data = new std::wstring(temp_path);
        result ? delegate_->OnAsyncComplete(AsyncTaskType::TASK_DOWNLOAD_COVER, AsyncErrorCode::ERROR_SUCCESS, data) :
            delegate_->OnAsyncComplete(AsyncTaskType::TASK_DOWNLOAD_COVER, AsyncErrorCode::ERROR_NETWORK_UNAVAILABLE, nullptr);
    }
}

void AsyncService::OnGetPlayerUrl(const std::shared_ptr<IAsyncTask>& task)
{
    auto http_task = dynamic_pointer_cast<HttpTask>(task);
    if (http_task == nullptr) {
        LOG(ERROR) << "OnGetPlayerUrl 转换AsyncTask对象失败";
        return;
    }
    auto response = DoHttpRequest(http_task);
    if (response.empty()) {
        return;
    }
    // 数据解析
    bool result = ParseUgcPlayerUrl(http_task, response);
    if (!result && delegate_) {
        delegate_->OnAsyncComplete(http_task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
    }
}

void AsyncService::OnDecodeVideo(const std::shared_ptr<IAsyncTask>& task)
{
    auto decode_task = dynamic_pointer_cast<DecodeTask>(task);
    if (decode_task == nullptr) {
        LOG(ERROR) << "OnDecodeVideo 转换DecodeTask对象失败";
        return;
    }
    // 在线程中同步解码视频
    AUDIO_SERVICE()->Decode(decode_task->task_id, decode_task->video_path, decode_task->mp3_path);
}

std_str AsyncService::DoHttpRequest(const std::shared_ptr<HttpTask>& task)
{
    RestClient::Connection http;
    InitHttp(http);
    auto response = http.get(task->url);
    if (response.curl_code != CURLE_OK) {
        LOG(ERROR) << "DoHttpRequest network error, code: " << response.curl_code;
        if (delegate_) {
            delegate_->OnAsyncComplete(task->task_type, AsyncErrorCode::ERROR_NETWORK_UNAVAILABLE, nullptr);
        }
        return "";
    }
    return response.body;
}

bool AsyncService::ParseUgcInfo(const std::shared_ptr<HttpTask>& task, std_cstr_ref response)
{
    // 数据源：https://api.bilibili.com/x/web-interface/view?bvid=1z44y137pK
    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(response, root)) {
            LOG(ERROR) << "ParseUgcInfo parse response failed, response: " << response;
            return false;
        }
        int code = root[kHttpResponseCode].asInt();
        auto msg = root[kHttpResponseMessage].asString();
        if (code != 0) {
            LOG(ERROR) << "ParseUgcInfo response error msg: " << msg;
            return false;
        }
        auto& data = root[kHttpResponseData];
        auto bvid = data["bvid"].asString();
        auto aid = data["aid"].asInt64();
        auto pic = data["pic"].asString();
        auto title = data["title"].asString();
        auto duration = data["duration"].asInt();
        auto author = data["owner"]["name"].asString();
        auto ctime = data["ctime"].asInt64();
        // 获取 cid
        auto& pages = data["pages"];
        if (!pages.isArray() || pages.size() < 1) {
            LOG(ERROR) << "ParseUgcInfo parse pages info failed, response: " << response;
            return false;
        }
        auto cid = pages[0]["cid"].asInt64();
        if (delegate_) {
            VideoInfo* userData = new VideoInfo(std::move(bvid), string_utils::Utf8ToU(title), 
                string_utils::Utf8ToU(author), aid, cid, duration, ctime);
            delegate_->OnAsyncComplete(task->task_type, AsyncErrorCode::ERROR_SUCCESS, (void*)userData);
            // 下载封面图片
            AddHttpTask(AsyncTaskType::TASK_DOWNLOAD_COVER, task->video_type, pic);
            // 获取视频下载地址
            auto url = BuildPlayerUrl(task->video_type, aid, cid);
            AddHttpTask(AsyncTaskType::TASK_GET_PLAYER_URL, task->video_type, url);
        }
        return true;
    }
    catch (...) {
        LOG(ERROR) << "ParseUgcInfo parse response failed, response: " << response;
    }
    return false;
}

bool AsyncService::ParseUgcPlayerUrl(const std::shared_ptr<HttpTask>& task, std_cstr_ref response)
{
    // 数据源：https://api.bilibili.com/x/player/playurl?avid=980706249&cid=577595237&qn=0&fourk=1
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::shared_ptr<Json::CharReader> rd(builder.newCharReader());
        std_str error_msg;
        if (!rd->parse(response.c_str(), response.c_str()+response.size(), &root, &error_msg)) {
            LOG(ERROR) << "ParseUgcVideoUrl parse json failed, data: " << response;
            return false;
        }
        int code = root[kHttpResponseCode].asInt();
        auto msg = root[kHttpResponseMessage].asString();
        if (code != 0) {
            LOG(ERROR) << "ParseUgcVideoUrl parse json failed, code: " << code;
            return false;
        }
        auto& data = root[kHttpResponseData];
        auto& desc_list = data["accept_description"];
        auto& quality_list = data["accept_quality"];
        if (!desc_list.isArray() || !quality_list.isArray() || desc_list.size() != quality_list.size()) {
            LOG(ERROR) << "ParseUgcVideoUrl parse accept_quality failed";
            return false;
        }
        std::vector<QualityInfo> qn_array;
        size_t count = desc_list.size();
        qn_array.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            qn_array.emplace_back(QualityInfo(quality_list[i].asInt(), desc_list[i].asString()));
        }
        std::sort(qn_array.begin(), qn_array.end(), [](QualityInfo& left, QualityInfo& right) {
            return left.value > right.value;
            });
        auto& durl = data["durl"];
        auto url = durl[0]["url"].asCString();
        auto encode = string_utils::UrlEncode(url);
        // 解析完毕，回调通知界面
        if (delegate_) {
            PlayerInfo* userData = new PlayerInfo(std::move(url), std::move(qn_array));
            delegate_->OnAsyncComplete(task->task_type, AsyncErrorCode::ERROR_SUCCESS, userData);
        }
    }
    catch (...) {
        LOG(ERROR) << "ParseUgcVideoUrl parse json failed";
    }
    return true;
}
