#pragma once
#include <curl/curl.h>
#include "ThreadMgr.h"
#include <map>
#include <string>
#include <vector>
using std::wstring;
using std::string;
using std::map;
using std::vector;

namespace httplib
{
    void InitializeLibCurl();
    void UninitializeLibCurl();

    class HttpRequest
    {
    public:
        HttpRequest(void);
        ~HttpRequest(void);

    public:
        BOOL Init(LPCTSTR str_SessionName);
        /// 添加HTTP请求头
        void AddHeader(const string& strKey, const string& strValue);

        /// 批量添加HTTP请求头
        void AddHeader(const std::map<string, string>& headerMap);

        void SetCookie(LPCSTR pszCookies);

        void SaveCookieFile(LPCSTR pszFile);

        void SetCookieFile(LPCSTR pszFile);

        DWORD GetContentLength();

        BOOL RecvContent(string& strResult);

        // 保存到文件
        BOOL RecvContentToFile(LPCTSTR strUrl, LPCTSTR strPath);

        string RecvRedirectLocation();

        string GetRealUrl();

        void GetErrorBuff(string& sBuff);

    public:
        virtual void SetTimeout(int seconds);
        virtual void SetMaxRetry(int nMaxRetry);
        virtual void SetHttpPort(int wPort);

        virtual BOOL Request(LPCTSTR strUrl, string strProxy = "");

        virtual DWORD GetStatusCode();

        virtual BOOL AddSendData(LPCSTR strSendData) { return FALSE; };
        virtual BOOL AddSendData(LPCWSTR lpszKey, LPCWSTR lpszValue) { return FALSE; };
        virtual BOOL AddSendData(LPCSTR lpszKey, LPCSTR lpszValue) { return FALSE; };

        static void GlobalInit();
        static void GlobalUnInit();
    protected:
        CURL* m_Curl;
        struct curl_slist* m_Headerlist;

        string m_sResult;
        string m_sHeader;
        string m_sUrl;
        string m_sRedirectUrl;
        string m_sCookie;
        string m_sCookieFile;
        DWORD  m_dwNetCode;
        char   m_pErrorBuff[CURL_ERROR_SIZE];

        int m_nTimeout;
        int m_nMaxRetry;
    };

    class HttpRequestGet : public HttpRequest
    {
    public:
        virtual BOOL Request(LPCTSTR strUrl, string strProxy = "");
    };

    class HttpRequestPost : public HttpRequest
    {
    public:

        virtual BOOL Request(LPCTSTR strUrl, const string& strProxy = "");

        BOOL AddSendData(LPCSTR strSendData);

    protected:
        string m_PostData;

    };

    class HttpRequestPostForm : public HttpRequestPost
    {
    public:
        virtual BOOL Request(LPCTSTR strUrl, string strProxy = "");

        BOOL AddSendData(LPCSTR lpszKey, LPCSTR lpszValue);
    };

    class HttpRequestPost_FormData : public HttpRequestPost
    {
    public:
        HttpRequestPost_FormData();
        ~HttpRequestPost_FormData();
        virtual BOOL Request(LPCTSTR strUrl, string strProxy = "");
        BOOL AddSendFile(const string& strKey, const string& strFilePath);
        BOOL AddSendData(const string& strKey, const string& strValue);

    private:
        curl_httppost* m_pFormDataPost;
    };

    typedef enum ENUM_DOWNLOAD_STATE
    {
        STATE_DOWNLOADING = 0,
        STATE_DOWNLOAD_HAS_FINISHED,
        STATE_DOWNLOAD_HAS_FAILED,
        STATE_DOWNLOAD_HAS_STOPED,
    }DownloadState;

    typedef void (*PROCESS_CALLBACK)(DownloadState state, double dltotal, double dlnow, void* Userdata);
    typedef void (*WRITE_DATA_CALLBACK)(void* buffer, size_t size);

    class CHttpDownload : public HttpRequestPost, public CThread
    {
    public:
        CHttpDownload(bool async = true);
        CHttpDownload(const CHttpDownload& obj);
        CHttpDownload(CHttpDownload&& obj);
        CHttpDownload& operator=(const CHttpDownload& obj);
        CHttpDownload& operator=(CHttpDownload&& obj);

        virtual ~CHttpDownload();

        //dwMaxDownSpeed 下载限速  单位是KB/s
        bool Initialize(const string& url, const wstring& path);
        // 下载限速  单位是KB/s
        void SetMaxSpeed(DWORD maxSpeed);
        // 下载回调
        void SetProcessCallback(PROCESS_CALLBACK callback, void* param);

        bool Start();
        bool Stop();		//等待下载线程真正结束后，在返回
        bool NeedStop();	//立即返回
        DWORD	GetErrorCode();
        const std::wstring& GetSavePath() const;

    protected:
        bool InitLibCurlConfig(bool append);
        void CloseFile();
        void ClearCurl();
        bool GetUrlFileSize();
        DownloadState Download();

        int	Run(void* argument) override;
        static size_t	DownFileCallback(void* buffer, size_t size, size_t nmemb, void* user_data);
        static int		ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
        static size_t	CurlReadCallback(char* buffer, size_t size, size_t nitems, void* instream);
        static int		CurlSockoptCallback(void* clientp, curl_socket_t curlfd, curlsocktype purpose);

    private:
        CURL* m_Curl = NULL;
        DWORD m_dwCode = 0;
        CURLcode m_dwCurlCode = CURLE_OK;
        HANDLE m_hFile = INVALID_HANDLE_VALUE;

        unsigned long m_ulStartPos = 0;
        BOOL m_bNeedStop;
        BOOL m_bHasStart;
        DWORD m_maxDownSpeed = 0;
        double m_UrlFileSize = 0.0F;
        curl_off_t m_FileCurrentSize = 0;
        void* m_userdata = NULL;
        PROCESS_CALLBACK m_processCallback = NULL;
        bool m_asyncMode = true;
        std::string m_url;
        std::wstring m_savePath;
    };
}