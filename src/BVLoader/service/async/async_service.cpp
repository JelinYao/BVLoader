#include "pch.h"
#include "async_service.h"
#include "async_task.h"
#include "async_service_delegate.h"
#include "url_parse.h"
#include <utils/HttpLib.h>
#include "soft_define.h"
#include "service_manager.h"

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
    ClearDelegate();
    task_event_.notify_one();
    if (async_thread_->joinable()) {
        async_thread_->join();
    }
}

void AsyncService::InitHttp(RestClient::Connection& http)
{
    http.SetUserAgent(kDefaultUserAgent);
    http.SetConnectTimeout(kDefaultHttpTimeOut);
    http.SetTimeout(kDefaultHttpTimeOut);
    http.AppendHeader("Referer", kDeafultReferer);
    if (!user_cookie_.empty()) {
        http.AppendHeader("cookie", user_cookie_);
    }
}

void AsyncService::AddDelegate(IAsyncServiceDelegate* delegate, void* param)
{
    std::lock_guard<std::mutex> lock(delegate_mutex_);
    delegate_list_[delegate] = param;
}

void AsyncService::RemoveDelegate(IAsyncServiceDelegate* delegate)
{
    std::lock_guard<std::mutex> lock(delegate_mutex_);
    auto iter = delegate_list_.find(delegate);
    if (iter != delegate_list_.end()) {
        delegate_list_.erase(iter);
    }
}

void AsyncService::AddHttpTask(AsyncTaskType task_type, std_cstr_ref url)
{
    if (url.empty()) {
        LOG(ERROR) << "AddHttpTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<HttpTask>(task_type, url));
    task_event_.notify_one();
}

void AsyncService::AddVideoTask(AsyncTaskType task_type, VideoType video_type, std_cstr_ref url)
{
    if (url.empty()) {
        LOG(ERROR) << "AddVideoTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<VideoTask>(task_type, video_type, url));
    task_event_.notify_one();
}

void AsyncService::AddDecodeTask(UINT_PTR task_id, std_cwstr_ref video_path, 
    std_cwstr_ref mp3_path, std_cwstr_ref img_path)
{
    if (video_path.empty() || mp3_path.empty()) {
        LOG(ERROR) << "AddDecodeTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<DecodeTask>(task_id, video_path, mp3_path, img_path));
    task_event_.notify_one();
}

void AsyncService::AddLoginTask(std_cstr_ref url, std_cstr_ref auth_key)
{
    if (url.empty() || auth_key.empty()) {
        LOG(ERROR) << "AddLoginTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<LoginTask>(url, auth_key));
    task_event_.notify_one();
}

void AsyncService::AddDownloadImageTask(ImageType image_type, std_cstr_ref url)
{
    if (url.empty()) {
        LOG(ERROR) << "AddDownloadImageTask invalid params";
        return;
    }
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_list_.emplace_back(std::make_shared<ImageTask>(image_type, url));
    task_event_.notify_one();
}

void AsyncService::SetCookie(std_cstr_ref cookie)
{
    user_cookie_ = cookie;
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
        case AsyncTaskType::TASK_DOWNLOAD_IMAGE:
            OnDownloadImage(task);
            break;
        case AsyncTaskType::TASK_GET_PLAYER_URL:
        case AsyncTaskType::TASK_GET_SELECT_PLAYER_URL:
            OnGetPlayerUrl(task);
            break;
        case AsyncTaskType::TASK_DECODE_VIDEO:
            OnDecodeVideo(task);
            break;
        case AsyncTaskType::TASK_GET_LOGIN_URL:
            OnGetLoginUrl(task);
            break;
        case AsyncTaskType::TASK_GET_LOGIN_INFO:
            OnGetLoginInfo(task);
            break;
        case AsyncTaskType::TASK_GET_USER_INFO:
            OnGetUserInfo(task);
            break;
        default:
            break;
        }
    }
}

void AsyncService::OnGetVideoInfo(const std::shared_ptr<IAsyncTask>& task)
{
    auto video_task = dynamic_pointer_cast<VideoTask>(task);
    if (video_task == nullptr) {
        LOG(ERROR) << "OnGetVideoInfo 转换AsyncTask对象失败";
        return;
    }
    auto response = DoHttpRequest(video_task);
    if(response.empty()) {
        return;
    }
    // 数据解析
    bool result = false;
    switch (video_task->video_type)
    {
    case VideoType::VIDEO_UGC: {
        result = (task->task_type == AsyncTaskType::TASK_GET_INFO) ?
            ParseUgcInfo(video_task, response) : ParseUgcPlayerUrl(video_task, response);
        break;
    }
    default:
        break;
    }
    if (!result && !IsDelegateEmpty()) {
        NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
    }
}

void AsyncService::OnDownloadImage(const std::shared_ptr<IAsyncTask>& task)
{
    auto image_task = dynamic_pointer_cast<ImageTask>(task);
    if (image_task == nullptr) {
        LOG(ERROR) << "OnDownloadCover 转换AsyncTask对象失败";
        return;
    }
    httplib::CHttpDownload http(false);
    http.AddHeader("Referer", kDeafultReferer);
    auto temp_path = system_utils::GetAppTempPathW(kAppName);
    temp_path += string_utils::GetRandomText(10);
    temp_path.append(L".jpg");
    http.Initialize(image_task->url, temp_path);
    bool result = http.Start();
    if (!IsDelegateEmpty()) {
        if (result) {
            ImageInfo* info = new ImageInfo(image_task->image_type, std::move(temp_path));
            NotifyDelegate(AsyncTaskType::TASK_DOWNLOAD_IMAGE, AsyncErrorCode::ERROR_SUCCESS, info);
        }
        else {
            NotifyDelegate(AsyncTaskType::TASK_DOWNLOAD_IMAGE, AsyncErrorCode::ERROR_NETWORK_UNAVAILABLE, nullptr);
        }   
    }
}

void AsyncService::OnGetPlayerUrl(const std::shared_ptr<IAsyncTask>& task)
{
    auto response = DoHttpRequest(task);
    if (response.empty()) {
        return;
    }
    // 数据解析
    bool result = ParseUgcPlayerUrl(task, response);
    if (!result && !IsDelegateEmpty()) {
        NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
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
    AUDIO_SERVICE()->Decode(decode_task->task_id, decode_task->video_path, 
        decode_task->mp3_path, decode_task->img_path);
}

void AsyncService::OnGetLoginUrl(const std::shared_ptr<IAsyncTask>& task)
{
    auto response = DoHttpRequest(task);
    if (response.empty()) {
        return;
    }
    // 数据解析
    bool result = ParseLoginUrl(task, response);
    if (!result && !IsDelegateEmpty()) {
        NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
    }
}

void AsyncService::OnGetLoginInfo(const std::shared_ptr<IAsyncTask>& task)
{
    auto login_task = dynamic_pointer_cast<LoginTask>(task);
    if (login_task == nullptr) {
        LOG(ERROR) << "OnGetLoginInfo 转换AsyncTask对象失败";
        return;
    }
    RestClient::Connection http;
    InitHttp(http);
    std_str param("oauthKey=");
    param += login_task->auth_key;
    auto response = http.post(login_task->url, param);
    if (response.curl_code != CURLE_OK) {
        LOG(ERROR) << "DoHttpRequest network error, code: " << response.curl_code;
        if (!IsDelegateEmpty()) {
            NotifyDelegate(login_task->task_type, AsyncErrorCode::ERROR_NETWORK_UNAVAILABLE, nullptr);
        }
        return;
    }
    // 数据解析
    bool result = ParseLoginInfo(task, response.body);
    if (!result && !IsDelegateEmpty()) {
        NotifyDelegate(login_task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
    }
}

void AsyncService::OnGetUserInfo(const std::shared_ptr<IAsyncTask>& task)
{
    auto response = DoHttpRequest(task);
    if (response.empty()) {
        return;
    }
    // 数据解析
    bool result = ParseUserInfo(task, response);
    if (!result && !IsDelegateEmpty()) {
        NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_PARSE_RESPONSE, nullptr);
    }
}

std_str AsyncService::DoHttpRequest(const std::shared_ptr<IAsyncTask>& task)
{
    auto http_task = dynamic_pointer_cast<HttpTask>(task);
    if (http_task == nullptr) {
        LOG(ERROR) << "DoHttpRequest 转换AsyncTask对象失败";
        return "";
    }
    RestClient::Connection http;
    InitHttp(http);
    auto response = http.get(http_task->url);
    if (response.curl_code != CURLE_OK) {
        LOG(ERROR) << "DoHttpRequest network error, code: " << response.curl_code;
        if (!IsDelegateEmpty()) {
            NotifyDelegate(http_task->task_type, AsyncErrorCode::ERROR_NETWORK_UNAVAILABLE, nullptr);
        }
        return "";
    }
    return response.body;
}

bool AsyncService::ParseUgcInfo(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response)
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
        if (!IsDelegateEmpty()) {
            VideoInfo* userData = new VideoInfo(std::move(bvid), string_utils::Utf8ToU(title), 
                string_utils::Utf8ToU(author), aid, cid, duration, ctime);
            NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_SUCCESS, (void*)userData);
            auto video_task = dynamic_pointer_cast<VideoTask>(task);
            if (video_task == nullptr) {
                LOG(ERROR) << "ParseUgcInfo 转换AsyncTask对象失败";
                return false;
            }
            // 下载封面图片
            AddDownloadImageTask(ImageType::IMAGE_VIDEO_COVER, pic);
            // 获取视频下载地址
            auto url = BuildPlayerUrl(video_task->video_type, aid, cid);
            AddHttpTask(AsyncTaskType::TASK_GET_PLAYER_URL, url);
        }
        return true;
    }
    catch (...) {
        LOG(ERROR) << "ParseUgcInfo parse response failed, response: " << response;
    }
    return false;
}

bool AsyncService::ParseUgcPlayerUrl(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response)
{
    // 数据源：https://api.bilibili.com/x/player/playurl?avid=980706249&cid=577595237&qn=0&fourk=1
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::shared_ptr<Json::CharReader> rd(builder.newCharReader());
        std_str error_msg;
        if (!rd->parse(response.c_str(), response.c_str()+response.size(), &root, &error_msg)) {
            LOG(ERROR) << "ParseUgcPlayerUrl parse json failed, data: " << response;
            return false;
        }
        int code = root[kHttpResponseCode].asInt();
        auto msg = root[kHttpResponseMessage].asString();
        if (code != 0) {
            LOG(ERROR) << "ParseUgcPlayerUrl parse json failed, code: " << code;
            return false;
        }
        auto& data = root[kHttpResponseData];
        auto& desc_list = data["accept_description"];
        auto& quality_list = data["accept_quality"];
        if (!desc_list.isArray() || !quality_list.isArray() || desc_list.size() != quality_list.size()) {
            LOG(ERROR) << "ParseUgcPlayerUrl parse accept_quality failed";
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
        if (!IsDelegateEmpty()) {
            PlayerInfo* userData = new PlayerInfo(std::move(url), std::move(qn_array));
            NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_SUCCESS, userData);
        }
        return true;
    }
    catch (...) {
        LOG(ERROR) << "ParseUgcPlayerUrl parse json failed";
    }
    return false;
}

bool AsyncService::ParseLoginUrl(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response)
{
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::shared_ptr<Json::CharReader> rd(builder.newCharReader());
        std_str error_msg;
        if (!rd->parse(response.c_str(), response.c_str() + response.size(), &root, &error_msg)) {
            LOG(ERROR) << "ParseLoginUrl parse json failed, data: " << response;
            return false;
        }
        int code = root[kHttpResponseCode].asInt();
        if (code != 0) {
            LOG(ERROR) << "ParseLoginUrl parse json failed, code: " << code;
            return false;
        }
        auto& data = root[kHttpResponseData];
        auto url = data["url"].asString();
        auto auth_key = data["oauthKey"].asString();
        // 解析完毕，回调通知界面
        if (!IsDelegateEmpty()) {
            QrcodeUrlInfo* userData = new QrcodeUrlInfo(std::move(url), std::move(auth_key));
            NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_SUCCESS, userData);
        }
    }
    catch (...) {
        LOG(ERROR) << "ParseLoginUrl parse json failed";
    }
    return false;
}

bool AsyncService::ParseLoginInfo(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response)
{
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::shared_ptr<Json::CharReader> rd(builder.newCharReader());
        std_str error_msg;
        if (!rd->parse(response.c_str(), response.c_str() + response.size(), &root, &error_msg)) {
            LOG(ERROR) << "ParseLoginInfo parse json failed, data: " << response;
            return false;
        }
        auto& data = root[kHttpResponseData];
        int code = 0;
        std_str url;
        std_str cookie;
        if (data.isInt()) {
            code = data.asInt();
        }
        else {
            // 登陆成功，解析用户信息
            url = data["url"].asString();
            size_t pos = url.find('?');
            if (pos != std::string::npos) {
                url = url.substr(pos + 1);
            }
            vector<string> param_list;
            string_utils::SplitStringA(url, "&", param_list);
            size_t count = param_list.size();
            for (size_t i = 0; i < count; ++i) {
                cookie += param_list[i];
                if (i != count - 1) {
                    cookie.append("; ");
                }
            }
        }
        // 解析完毕，回调通知界面
        if (!IsDelegateEmpty()) {
            QrcodeLoginInfo* login_info = new QrcodeLoginInfo(code, std::move(url), std::move(cookie));
            NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_SUCCESS, login_info);
        }
        return true;
    }
    catch (...) {
        LOG(ERROR) << "ParseLoginInfo parse json failed";
    }
    return false;
}

bool AsyncService::ParseUserInfo(const std::shared_ptr<IAsyncTask>& task, std_cstr_ref response)
{
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::shared_ptr<Json::CharReader> rd(builder.newCharReader());
        std_str error_msg;
        if (!rd->parse(response.c_str(), response.c_str() + response.size(), &root, &error_msg)) {
            LOG(ERROR) << "ParseUserInfo parse json failed, data: " << response;
            return false;
        }
        int code = root[kHttpResponseCode].asInt();
        auto msg = root[kHttpResponseMessage].asString();
        if (code != 0) {
            LOG(ERROR) << "ParseUserInfo response error msg: " << msg;
            return false;
        }
        auto& data = root[kHttpResponseData];
        auto avatar = data["face"].asString();
        auto name = data["uname"].asString();
        auto& level = data["level_info"];
        // 解析完毕，回调通知界面
        if (!IsDelegateEmpty()) {
            UserInfo* user_info = new UserInfo();
            user_info->name = string_utils::Utf8ToU(data["uname"].asString());
            user_info->current_level = level["current_level"].asInt();
            user_info->current_min = level["current_min"].asInt();
            user_info->current_exp = level["current_exp"].asInt();
            user_info->next_exp = level["next_exp"].asInt();
            user_info->money = data["money"].asInt();
            NotifyDelegate(task->task_type, AsyncErrorCode::ERROR_SUCCESS, user_info);
            // 下载用户头像
            AddDownloadImageTask(ImageType::IMAGE_VIDEO_AVATAR, avatar);
        }
        return true;
    }
    catch (...) {
        LOG(ERROR) << "ParseUserInfo parse json failed";
    }
    return false;
}

void AsyncService::NotifyDelegate(AsyncTaskType task_type, AsyncErrorCode code, void* data)
{
    std::lock_guard<std::mutex> lock(delegate_mutex_);
    for (auto& delegate : delegate_list_) {
        delegate.first->OnAsyncComplete(task_type, code, data, delegate.second);
    }
}

void AsyncService::ClearDelegate()
{
    std::lock_guard<std::mutex> lock(delegate_mutex_);
    delegate_list_.clear();
}
