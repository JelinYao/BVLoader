#pragma once

// UGC��Ƶ�����ѯ�ӿ�
constexpr const char* kUgcDetailApi = "https://api.bilibili.com/x/web-interface/view?bvid=";
// UGC��Ƶ����ҳ��ѯ�ӿ�
constexpr const char* kUgcPlayerApi = "https://api.bilibili.com/x/player/playurl?avid=%I64d&cid=%I64d&qn=%d&fourk=1";
// �����ά������
// static constexpr const char* kRequestLoginUrl = "https://passport.bilibili.com/qrcode/getLoginUrl";
// �°汾
static constexpr const char* kRequestLoginUrl = "https://passport.bilibili.com/x/passport-login/web/qrcode/generate?source=main-fe-header";
// �����¼���
// static constexpr const char* kRequestLoginInfo = "https://passport.bilibili.com/qrcode/getLoginInfo";
// �°汾
static constexpr const char* kRequestLoginInfo = "https://passport.bilibili.com/x/passport-login/web/qrcode/poll?qrcode_key=";
// ��ȡ�û���Ϣ
static constexpr const char* kRequestUserInfo = "https://api.bilibili.com/x/web-interface/nav";