#pragma once

static constexpr const wchar_t* kAppWindowClassName = L"GuiFoundationClass";
static constexpr const wchar_t* kAppWindowTitle = L"B站视频下载工具";
static constexpr const wchar_t* kAppInstanceMutex = L"{E33E6D86-55E3-496B-B961-A205582F84EC}";
static constexpr const wchar_t* kAppExeName = L"BVLoader.exe";
static constexpr const wchar_t* kAppVersion = L"1.0.1";
static constexpr const wchar_t* kAppName = L"DownloadTools";
static constexpr const char* kAppNameAscii = "DownloadTools";

static constexpr const wchar_t* kWndParseTitle = L"视频信息";
static constexpr const wchar_t* kWndMsgTitle = L"提示窗口";
static constexpr const wchar_t* kWndLoginTitle = L"登录";

// 定义常量文字
// download
static constexpr const wchar_t* kTextLoading = L"下载中";
static constexpr const wchar_t* kTextLoadPause = L"暂停下载";
static constexpr const wchar_t* kTextLoadWaiting = L"等待下载";
static constexpr const wchar_t* kTextLoadFailed = L"下载失败";
static constexpr const wchar_t* kTextLoadFinish = L"下载完成";
static constexpr const wchar_t* kTextDecoding = L"解码中";
// qrcode login
static constexpr const wchar_t* kTextQrcodeExpired = L"二维码已过期";
static constexpr const wchar_t* kTextQrcodeRequestFailed = L"二维码请求失败";
static constexpr const wchar_t* kTextQrcodeConfirmed = L"扫描成功，请在手机上确认";

static constexpr const wchar_t* kDefaultCover = L"cover.png";