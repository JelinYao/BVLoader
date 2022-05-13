#pragma once

// UGC视频详情查询接口
constexpr const char* kUgcDetailApi = "https://api.bilibili.com/x/web-interface/view?bvid=";
// UGC视频播放页查询接口
constexpr const char* kUgcPlayerApi = "https://api.bilibili.com/x/player/playurl?avid=%I64d&cid=%I64d&qn=%d&fourk=1";
// 请求二维码链接
static constexpr const char* kRequestLoginUrl = "https://passport.bilibili.com/qrcode/getLoginUrl";
// 请求登录结果
static constexpr const char* kRequestLoginInfo = "https://passport.bilibili.com/qrcode/getLoginInfo";
// 获取用户信息
static constexpr const char* kRequestUserInfo = "https://api.bilibili.com/nav";