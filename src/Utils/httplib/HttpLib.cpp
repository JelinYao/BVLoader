#include "HttpLib.h"
#include <sys/stat.h>
#include  <io.h>
#include <utils/string_utils.h>
#include <utils/easylogging++.h>
#include <shlwapi.h>

namespace httplib
{
    static const char* kDefaultUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.75 Safari/537.36 Edg/100.0.1185.36";
    void InitializeLibCurl()
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    void UninitializeLibCurl()
    {
        curl_global_cleanup();
    }

    size_t NetCallback(void* ptr, size_t size, size_t nmemb, void* stream)
    {
        int len = size * nmemb;
        int written = len;
        string* sCxt = (string*)stream;
        if (sCxt)
        {
            sCxt->append((const char*)ptr, len);
        }

        return written;
    }

    size_t HeaderCallback(void* ptr, size_t size, size_t nmemb, void* stream)
    {
        int len = size * nmemb;
        int written = len;
        string* sCxt = (string*)stream;
        if (sCxt)
        {
            sCxt->append((const char*)ptr, len);
        }

        return written;
    }

    size_t DownFileCallback(void* buffer, size_t size, size_t nmemb, void* user_data)
    {
        if (NULL == user_data)
        {
            return 0;
        }

        FILE* f = (FILE*)user_data;
        int nWrite = fwrite(buffer, size, nmemb, f);
        fflush(f);

        return nWrite;
    }


    HttpRequest::HttpRequest(void) :
        m_Curl(NULL),
        m_Headerlist(NULL),
        m_dwNetCode(200),
        m_nTimeout(60),
        m_nMaxRetry(3)
    {
        memset(m_pErrorBuff, 0, CURL_ERROR_SIZE);
    }

    HttpRequest::~HttpRequest(void)
    {
    }

    BOOL HttpRequest::Init(LPCTSTR str_SessionName)
    {
        //curl_global_init(CURL_GLOBAL_DEFAULT);
        m_Curl = curl_easy_init();

        return true;
    }

    void HttpRequest::AddHeader(const string& strKey, const string& strValue)
    {
        string strHeader(strKey);
        strHeader.append(":");
        strHeader.append(strValue);
        m_Headerlist = curl_slist_append(m_Headerlist, strHeader.c_str());
    }

    void HttpRequest::AddHeader(const std::map<string, string>& headerMap)
    {
        typedef std::map<string, string>::const_iterator HeadMapIt;
        HeadMapIt it = headerMap.begin();
        for (; it != headerMap.end(); ++it)
        {
            AddHeader(it->first, it->second);
        }
    }

    DWORD HttpRequest::GetContentLength()
    {
        return m_sResult.length();
    }

    BOOL HttpRequest::RecvContentToFile(LPCTSTR strUrl, LPCTSTR strPath)
    {
        string sUrl = string_utils::UToUtf8(strUrl);
        m_sUrl = sUrl;

        FILE* pFile = NULL;
        _wfopen_s(&pFile, strPath, L"wb+");
        if (NULL == pFile)
        {
            return FALSE;
        }

        curl_easy_setopt(m_Curl, CURLOPT_URL, sUrl.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, DownFileCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, pFile);
        curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, 60); //设置连接超时时间
        curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_pErrorBuff);

        CURLcode res = curl_easy_perform(m_Curl);

        fclose(pFile);

        if (NULL != m_Curl)
        {
            curl_easy_cleanup(m_Curl);
        }

        //curl_global_cleanup();

        if (res != CURLE_OK)
        {
            return FALSE;
        }

        return TRUE;
    }

    BOOL HttpRequest::RecvContent(string& strResult)
    {
        BOOL bSuccess = FALSE;
        int nTry = 0;
    Try:
        CURLcode res = curl_easy_perform(m_Curl);
        if (CURLE_OK != res)
        {
            m_dwNetCode = 909;
            curl_easy_strerror(res);
            m_sResult.clear();
            m_sHeader.clear();

            if (++nTry < m_nMaxRetry)
            {
                goto Try;
            }

        }
        else
        {
            strResult = m_sResult;

            DWORD dwCode = 0;
            CURLcode res;
            res = curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &dwCode);
            if (CURLE_OK == res)
            {
                m_dwNetCode = dwCode;
            }

            DWORD dwCount = 0;
            curl_easy_getinfo(m_Curl, CURLINFO_REDIRECT_COUNT, &dwCount);

            char redirectUrl[2048] = { 0 };
            if (dwCount > 0)
            {
                int nLen = sizeof("location:");
                size_t nPos = m_sHeader.find("location:");
                if (string::npos != nPos)
                {
                    nPos += nLen;
                    int n = 0;
                    char ch = m_sHeader.at(nPos++);
                    while (ch != '\n')
                    {
                        redirectUrl[n++] = ch;
                        ch = m_sHeader.at(nPos++);
                    }

                    m_sRedirectUrl = redirectUrl;
                }
            }

            if (NULL != m_Headerlist)
            {
                curl_slist_free_all(m_Headerlist);
                m_Headerlist = NULL;
            }
            bSuccess = TRUE;
        }

        curl_easy_cleanup(m_Curl);
        return bSuccess;
    }

    BOOL HttpRequest::Request(LPCTSTR strUrl, string strProxy)
    {
        string sUrl = string_utils::UToUtf8(strUrl);
        m_sUrl = sUrl;

        curl_easy_setopt(m_Curl, CURLOPT_PROXY, strProxy.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_URL, sUrl.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, m_Headerlist);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, NetCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_sResult);
        curl_easy_setopt(m_Curl, CURLOPT_HEADERFUNCTION, &HeaderCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEHEADER, &m_sHeader);
        curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1); //支持重定向
        curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, 180); //设置连接超时时间
        curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_pErrorBuff);
        curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, "");

        if (!m_sCookie.empty())
        {
            curl_easy_setopt(m_Curl, CURLOPT_COOKIE, m_sCookie.c_str());
        }

        if (NULL != m_Headerlist)
        {
            curl_easy_setopt(m_Curl, CURLOPT_POSTQUOTE, &m_Headerlist);
        }

        if (0 == _strnicmp(m_sUrl.c_str(), "https", 5))
        {
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        return TRUE;
    }

    DWORD HttpRequest::GetStatusCode()
    {
        return m_dwNetCode;
    }

    string HttpRequest::RecvRedirectLocation()
    {
        return m_sRedirectUrl;
    }

    string HttpRequest::GetRealUrl()
    {
        return m_sRedirectUrl;
    }

    void HttpRequest::SetCookie(LPCSTR pszCookies)
    {
        m_sCookie = pszCookies;
    }

    void HttpRequest::GetErrorBuff(string& sBuff)
    {
        sBuff = m_pErrorBuff;
    }

    void HttpRequest::SetTimeout(int seconds)
    {
        m_nTimeout = seconds;
    }

    void HttpRequest::SetMaxRetry(int nMaxRetry)
    {
        m_nMaxRetry = nMaxRetry;
    }

    void HttpRequest::SetHttpPort(int wPort)
    {
        curl_easy_setopt(m_Curl, CURLOPT_PORT, LONG(wPort));
    }

    void HttpRequest::SaveCookieFile(LPCSTR pszFile)
    {
        m_sCookieFile = pszFile;
        curl_easy_setopt(m_Curl, CURLOPT_COOKIEJAR, m_sCookieFile.c_str());
    }

    void HttpRequest::SetCookieFile(LPCSTR pszFile)
    {
        m_sCookieFile = pszFile;
        curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, m_sCookieFile.c_str());
    }

    void HttpRequest::GlobalInit()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    void HttpRequest::GlobalUnInit()
    {
        //curl_global_cleanup();
    }

    BOOL HttpRequestGet::Request(LPCTSTR strUrl, string strProxy)
    {
        return __super::Request(strUrl, strProxy);
    }

    BOOL HttpRequestPost::Request(LPCTSTR strUrl, const string& strProxy)
    {
        string sUrl = string_utils::UToUtf8(strUrl);
        m_sUrl = sUrl;

        curl_easy_setopt(m_Curl, CURLOPT_PROXY, strProxy.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_URL, sUrl.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, m_Headerlist);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, NetCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_sResult);
        curl_easy_setopt(m_Curl, CURLOPT_HEADERFUNCTION, &HeaderCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEHEADER, &m_sHeader);
        curl_easy_setopt(m_Curl, CURLOPT_POST, 1);
        curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, m_nTimeout); //设置连接超时时间
        curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_pErrorBuff);
        curl_easy_setopt(m_Curl, CURLOPT_ACCEPT_ENCODING, "");


        if (NULL != m_Headerlist)
        {
            curl_easy_setopt(m_Curl, CURLOPT_POSTQUOTE, &m_Headerlist);
        }

        if (0 == _strnicmp(m_sUrl.c_str(), "https", 5))
        {
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, m_PostData.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDSIZE, m_PostData.c_str());
        return TRUE;
    }

    BOOL HttpRequestPost::AddSendData(LPCSTR strSendData)
    {
        if (NULL == strSendData)
        {
            return FALSE;
        }

        m_PostData = strSendData;

        return TRUE;
    }

    BOOL HttpRequestPostForm::Request(LPCTSTR strUrl, string strProxy)
    {
        AddHeader("Content-Type", "application/x-www-form-urlencoded");
        return __super::Request(strUrl, strProxy);
    }

    BOOL HttpRequestPostForm::AddSendData(LPCSTR lpszKey, LPCSTR lpszValue)
    {
        if (m_PostData != "")
        {
            m_PostData += "&";
        }
        m_PostData += lpszKey;
        m_PostData += "=";
        m_PostData += lpszValue;

        return TRUE;
    }

    HttpRequestPost_FormData::HttpRequestPost_FormData()
    {
        m_pFormDataPost = NULL;
    }

    HttpRequestPost_FormData::~HttpRequestPost_FormData()
    {
        if (NULL != m_pFormDataPost)
        {
            curl_formfree(m_pFormDataPost);
            m_pFormDataPost = NULL;
        }
    }

    BOOL HttpRequestPost_FormData::Request(LPCTSTR strUrl, string strProxy /*= ""*/)
    {
        string sUrl = string_utils::UToUtf8(strUrl);
        m_sUrl = sUrl;

        curl_easy_setopt(m_Curl, CURLOPT_PROXY, strProxy.c_str());
        curl_easy_setopt(m_Curl, CURLOPT_URL, sUrl.c_str());
        m_Headerlist = curl_slist_append(m_Headerlist, "Expect:");
        curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, m_Headerlist);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, NetCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_sResult);
        curl_easy_setopt(m_Curl, CURLOPT_HEADERFUNCTION, &HeaderCallback);
        curl_easy_setopt(m_Curl, CURLOPT_WRITEHEADER, &m_sHeader);
        curl_easy_setopt(m_Curl, CURLOPT_POST, 1);
        curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, m_nTimeout); //设置连接超时时间
        curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_pErrorBuff);
        curl_easy_setopt(m_Curl, CURLOPT_ACCEPT_ENCODING, "");


        if (0 == _strnicmp(m_sUrl.c_str(), "https", 5))
        {
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        curl_easy_setopt(m_Curl, CURLOPT_HTTPPOST, m_pFormDataPost);
        return TRUE;
    }

    BOOL HttpRequestPost_FormData::AddSendFile(const string& strKey, const string& strFilePath)
    {
        curl_httppost* last = NULL;
        if (curl_formadd(&m_pFormDataPost, &last, CURLFORM_COPYNAME, strKey.c_str(),
            CURLFORM_FILE, strFilePath.c_str(), CURLFORM_END) != 0)
        {
            return FALSE;
        }
        return TRUE;
    }

    BOOL HttpRequestPost_FormData::AddSendData(const string& strKey, const string& strValue)
    {
        curl_httppost* last = NULL;
        if (curl_formadd(&m_pFormDataPost, &last, CURLFORM_COPYNAME, strKey.c_str(),
            CURLFORM_COPYCONTENTS, strValue.c_str(), CURLFORM_END) != 0)
        {
            return FALSE;
        }
        return TRUE;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHttpDownload::CHttpDownload(bool async/* = true*/)
        : m_Curl(NULL)
        , m_dwCode(0)
        , m_bHasStart(FALSE)
        , m_bNeedStop(FALSE)
        , m_ulStartPos(0)
        , m_hFile(INVALID_HANDLE_VALUE)
        , m_processCallback(NULL)
        , m_UrlFileSize(0)
        , m_FileCurrentSize(0)
        , m_asyncMode(async)
    {

    }

    CHttpDownload::CHttpDownload(const CHttpDownload& obj)
    {
        m_Curl = obj.m_Curl;
        m_dwCode = obj.m_dwCode;
        m_dwCurlCode = obj.m_dwCurlCode;
        m_hFile = obj.m_hFile;
        m_ulStartPos = obj.m_ulStartPos;
        m_bNeedStop = obj.m_bNeedStop;
        m_bHasStart = obj.m_bHasStart;
        m_maxDownSpeed = obj.m_maxDownSpeed;
        m_UrlFileSize = obj.m_UrlFileSize;
        m_FileCurrentSize = obj.m_FileCurrentSize;
        m_userdata = obj.m_userdata;
        m_processCallback = obj.m_processCallback;
        m_asyncMode = obj.m_asyncMode;
        m_url = obj.m_url;
        m_savePath = obj.m_savePath;
    }

    CHttpDownload::CHttpDownload(CHttpDownload&& obj)
    {
        m_Curl = obj.m_Curl;
        m_dwCode = obj.m_dwCode;
        m_dwCurlCode = obj.m_dwCurlCode;
        m_hFile = obj.m_hFile;
        m_ulStartPos = obj.m_ulStartPos;
        m_bNeedStop = obj.m_bNeedStop;
        m_bHasStart = obj.m_bHasStart;
        m_maxDownSpeed = obj.m_maxDownSpeed;
        m_UrlFileSize = obj.m_UrlFileSize;
        m_FileCurrentSize = obj.m_FileCurrentSize;
        m_userdata = obj.m_userdata;
        m_processCallback = obj.m_processCallback;
        m_asyncMode = obj.m_asyncMode;
        m_url = std::move(obj.m_url);
        m_savePath = std::move(obj.m_savePath);
    }

    CHttpDownload& CHttpDownload::operator=(const CHttpDownload& obj)
    {
        if (this != &obj) {
            m_Curl = obj.m_Curl;
            m_dwCode = obj.m_dwCode;
            m_dwCurlCode = obj.m_dwCurlCode;
            m_hFile = obj.m_hFile;
            m_ulStartPos = obj.m_ulStartPos;
            m_bNeedStop = obj.m_bNeedStop;
            m_bHasStart = obj.m_bHasStart;
            m_maxDownSpeed = obj.m_maxDownSpeed;
            m_UrlFileSize = obj.m_UrlFileSize;
            m_FileCurrentSize = obj.m_FileCurrentSize;
            m_userdata = obj.m_userdata;
            m_processCallback = obj.m_processCallback;
            m_asyncMode = obj.m_asyncMode;
            m_url = obj.m_url;
            m_savePath = obj.m_savePath;
        }
        return *this;
    }

    CHttpDownload& CHttpDownload::operator=(CHttpDownload&& obj)
    {
        if (this != &obj) {
            m_Curl = obj.m_Curl;
            m_dwCode = obj.m_dwCode;
            m_dwCurlCode = obj.m_dwCurlCode;
            m_hFile = obj.m_hFile;
            m_ulStartPos = obj.m_ulStartPos;
            m_bNeedStop = obj.m_bNeedStop;
            m_bHasStart = obj.m_bHasStart;
            m_maxDownSpeed = obj.m_maxDownSpeed;
            m_UrlFileSize = obj.m_UrlFileSize;
            m_FileCurrentSize = obj.m_FileCurrentSize;
            m_userdata = obj.m_userdata;
            m_processCallback = obj.m_processCallback;
            m_asyncMode = obj.m_asyncMode;
            m_url = std::move(obj.m_url);
            m_savePath = std::move(obj.m_savePath);
        }
        return *this;
    }

    CHttpDownload::~CHttpDownload()
    {
        ClearCurl();
        CloseFile();
    }

    size_t CHttpDownload::DownFileCallback(void* buffer, size_t size, size_t nmemb, void* user_data)
    {
        if (NULL == user_data)
        {
            return 0;
        }
        size_t dwBufferSize = size * nmemb;
        CHttpDownload* pThis = (CHttpDownload*)user_data;
        pThis->m_FileCurrentSize += dwBufferSize;
        if (pThis->m_hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwWriteSize = 0;
            if (!WriteFile(pThis->m_hFile, buffer, dwBufferSize, &dwWriteSize, NULL))
            {
                LOG(ERROR) << "写入文件失败，系统错误码：" << GetLastError();
                return 0;
            }
        }
        if (pThis->m_FileCurrentSize > pThis->m_UrlFileSize)
        {
            return 0;
        }
        if (pThis->m_bNeedStop)
        {
            return 0;
        }
        return dwBufferSize;
    }

    int CHttpDownload::ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
    {
        static double lastdlnow = 0;
        if (NULL == clientp)
        {
            return 0;
        }
        CHttpDownload* pThis = (CHttpDownload*)clientp;
        if (pThis->m_processCallback && (pThis->m_bNeedStop == FALSE))
        {
            double dltotaltmp = 0, dlnowtmp = 0;
            if (dltotal && lastdlnow != dlnow)
            {
                lastdlnow = dlnow;
                pThis->m_processCallback(STATE_DOWNLOADING, dltotal + pThis->m_ulStartPos, dlnow + pThis->m_ulStartPos, pThis->m_userdata);
            }
        }
        return 0;
    }

    size_t CHttpDownload::CurlReadCallback(char* buffer, size_t size, size_t nitems, void* instream)
    {
        if (NULL == instream)
        {
            return 0;
        }
        CHttpDownload* pThis = (CHttpDownload*)instream;
        if (pThis->m_bNeedStop)
        {
            return CURL_READFUNC_ABORT;
        }
        return 0;
    }

    int CHttpDownload::CurlSockoptCallback(void* clientp, curl_socket_t curlfd, curlsocktype purpose)
    {
        if (NULL == clientp)
        {
            return CURL_SOCKOPT_OK;
        }
        CHttpDownload* pThis = (CHttpDownload*)clientp;
        if (pThis->m_bNeedStop)
        {
            LOG(INFO) << "libcurl停止回调下载进度";
            return CURL_SOCKOPT_ERROR;
        }

        return CURL_SOCKOPT_OK;
    }

    bool CHttpDownload::InitLibCurlConfig(bool bAppend)
    {
        m_FileCurrentSize = 0;
        wstring strLocalPathTmp = m_savePath + L"_tmp";
        if (!bAppend && PathFileExists(strLocalPathTmp.c_str()))
        {
            DeleteFile(strLocalPathTmp.c_str());
        }
        m_hFile = CreateFile(strLocalPathTmp.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
        if (INVALID_HANDLE_VALUE == m_hFile)
        {
            LOG(ERROR) << "打开文件：" << string_utils::UToA(strLocalPathTmp) << "失败，系统错误码：" << GetLastError();
            return false;
        }
        LARGE_INTEGER size;
        ::GetFileSizeEx(m_hFile, &size);
        m_FileCurrentSize = size.QuadPart;
        if (m_FileCurrentSize > 0)
            SetFilePointer(m_hFile, 0, NULL, FILE_END);
        // 释放资源
        ClearCurl();
        m_Curl = curl_easy_init();
        // 设置属性
        if (curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK) return FALSE;//支持重定向
        if (curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, 1)) return FALSE;//CURL报告每一件意外的事情
        //if(curl_easy_setopt(m_Curl, CURLOPT_CONNECTTIMEOUT, 300)) return FALSE;//在发起连接前等待的时间 5分钟
        if (curl_easy_setopt(m_Curl, CURLOPT_HEADER, 0) != CURLE_OK) return FALSE;//不需要头文件信息
        //if(curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, 300) != CURLE_OK) return FALSE;//设置curl允许执行的最长5分钟
        if (curl_easy_setopt(m_Curl, CURLOPT_URL, m_url.c_str()) != CURLE_OK) return FALSE;//设置链接地址
        if (curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, CHttpDownload::DownFileCallback) != CURLE_OK) return FALSE;//设置写数据函数
        if (curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, this) != CURLE_OK) return FALSE;
        if (curl_easy_setopt(m_Curl, CURLOPT_NOPROGRESS, false) != CURLE_OK) return FALSE;
        if (curl_easy_setopt(m_Curl, CURLOPT_PROGRESSFUNCTION, CHttpDownload::ProgressCallback) != CURLE_OK) return FALSE;
        if (curl_easy_setopt(m_Curl, CURLOPT_PROGRESSDATA, this) != CURLE_OK) return FALSE;
        if (curl_easy_setopt(m_Curl, CURLOPT_RESUME_FROM_LARGE, m_FileCurrentSize) != CURLE_OK) return FALSE;
        if (curl_easy_setopt(m_Curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1) != CURLE_OK) return FALSE;
        curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, m_Headerlist);
        curl_easy_setopt(m_Curl, CURLOPT_USERAGENT, kDefaultUserAgent);

        if (0 == _strnicmp(m_url.c_str(), "https", 5))
        {
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        if (m_maxDownSpeed > 0)
        {
            curl_off_t MaxDownSpeed = (curl_off_t)(m_maxDownSpeed * 1024);
            if (curl_easy_setopt(m_Curl, CURLOPT_MAX_RECV_SPEED_LARGE, MaxDownSpeed) != CURLE_OK)
                return false;
        }

        m_ulStartPos = (unsigned long)m_FileCurrentSize;
        return true;
    }

    void CHttpDownload::CloseFile()
    {
        if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
    }

    void CHttpDownload::ClearCurl()
    {
        if (NULL != m_Curl) {
            curl_easy_cleanup(m_Curl);
            m_Curl = NULL;
        }
    }

    size_t GetUrlFileSizeWrtteFile(void* buffer, size_t size, size_t nmemb, void* user_data)
    {
        int len = size * nmemb;
        int written = len;

        return written;
    }

    bool CHttpDownload::GetUrlFileSize()
    {
        CURLcode	code;
        int			nReUrl = 0;
        CURL* curl = curl_easy_init();
        if (curl == NULL)
        {
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, m_Headerlist);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADER, 1);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GetUrlFileSizeWrtteFile);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, kDefaultUserAgent);

        if (0 == _strnicmp(m_url.c_str(), "https", 5)) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        // 执行操作，为获取下载文件长度做准备
        code = curl_easy_perform(curl);
        if (code != CURLE_OK)
        {
            return false;
        }

        // 获取下载文件长度
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &m_UrlFileSize) != CURLE_OK)return FALSE;

        DWORD dwCode = 0;
        if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &dwCode) != CURLE_OK)return FALSE;
        if (dwCode != 200) {
            LOG(ERROR) << "GetUrlFileSize response code: " << dwCode;
            return false;
        }

        curl_easy_cleanup(curl);
        return true;
    }

    DownloadState CHttpDownload::Download()
    {
        DownloadState dtate = STATE_DOWNLOAD_HAS_FAILED;
        while (true) {
            if (!InitLibCurlConfig(true))
            {
                LOG(ERROR) << "Download 初始化文件指针失败";
                dtate = STATE_DOWNLOAD_HAS_FAILED;
                break;
            }
            wstring strLocalPathTmp = m_savePath + L"_tmp";
            if (!GetUrlFileSize() || m_UrlFileSize == 0)
            {
                CloseFile();
                dtate = STATE_DOWNLOAD_HAS_FAILED;
                break;
            }
            if (m_FileCurrentSize >= (curl_off_t)m_UrlFileSize)
            {
                CloseFile();
                if (MoveFile(strLocalPathTmp.c_str(), m_savePath.c_str()))
                {
                    dtate = STATE_DOWNLOAD_HAS_FINISHED;
                }
                dtate = STATE_DOWNLOAD_HAS_FAILED;
                break;
            }
            m_dwCurlCode = CURLE_OK;
            m_dwCurlCode = curl_easy_perform(m_Curl);
            LARGE_INTEGER size = { 0 };
            ::GetFileSizeEx(m_hFile, &size);
            CloseFile();
            // 如果原文件存在，删掉
            if (PathFileExists(m_savePath.c_str())) {
                DeleteFile(m_savePath.c_str());
            }
            // 获取返回码
            curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_dwCode);
            if (m_bNeedStop)//外部取消了下载，不用去检测返回值来判断是否成功
            {
                dtate = STATE_DOWNLOAD_HAS_STOPED;
                break;
            }
            LOG(INFO) << "Download http response code: " << m_dwCode;
            if (m_dwCode == 404)
            {
                dtate = STATE_DOWNLOAD_HAS_FAILED;
                break;
            }

            if (m_dwCurlCode == CURLE_OK)
            {
                if (size.QuadPart == (LONGLONG)m_UrlFileSize && MoveFile(strLocalPathTmp.c_str(), m_savePath.c_str()))
                {//下载成功
                    dtate = STATE_DOWNLOAD_HAS_FINISHED;
                    break;
                }
                dtate = STATE_DOWNLOAD_HAS_FAILED;
                LOG(ERROR) << "Download 下载失败，文件大小不一致，系统错误码: " << GetLastError();
                break;
            }
            else if (m_dwCurlCode == CURLE_BAD_FUNCTION_ARGUMENT)
            {
                dtate = STATE_DOWNLOAD_HAS_STOPED;
                break;
            }
            dtate = STATE_DOWNLOAD_HAS_FAILED;
            break;
        }
        if (m_processCallback) {
            m_processCallback(dtate, 0, 0, m_userdata);
        }
        return dtate;
    }

    bool CHttpDownload::Initialize(const string& url, const wstring& path)
    {
        if (url.empty() || path.empty())
            return false;

        m_url = url;
        m_savePath = path;

        m_bNeedStop = FALSE;
        if (m_asyncMode) {
            CreateThread("下载文件线程", NULL);
        }
        return true;
    }

    void CHttpDownload::SetMaxSpeed(DWORD maxSpeed)
    {
        m_maxDownSpeed = maxSpeed;
    }

    void CHttpDownload::SetProcessCallback(PROCESS_CALLBACK callback, void* param)
    {
        m_processCallback = callback;
        m_userdata = param;
    }

    int CHttpDownload::Run(void* argument)
    {
        Download();
        return 0;
    }

    bool CHttpDownload::Start()
    {
        if (m_Curl == NULL && IsHasExited())
            return false;
        if (m_asyncMode) {
            if (m_bHasStart == FALSE)
            {
                ResumeThread();
                m_bHasStart = TRUE;
            }
            else
            {
                curl_easy_pause(m_Curl, CURLPAUSE_RECV_CONT);
            }
            return true;
        }
        return Download() == STATE_DOWNLOAD_HAS_FINISHED;
    }

    bool CHttpDownload::Stop()
    {
        if (m_Curl == NULL)
            return true;

        m_bNeedStop = TRUE;
        int nTry = 0;
        while (!IsHasExited() && (nTry < 100))
        {
            Sleep(50);
            nTry++;
        }

        if (nTry >= 100)
        {
            //如果等待5秒钟还是没有退出线程，只能强制解除进程了
            ::TerminateThread(GetThreadHandle(), 0);
            if (NULL != m_Curl)
            {
                curl_easy_cleanup(m_Curl);
                m_Curl = NULL;
            }
            return false;
        }

        m_Curl = NULL;
        return true;
    }

    bool CHttpDownload::NeedStop()
    {
        if (m_Curl == NULL) 
            return true;

        m_bNeedStop = TRUE;
        return false;
    }

    DWORD CHttpDownload::GetErrorCode()
    {
        return m_dwCode;
    }

    const std::wstring& CHttpDownload::GetSavePath() const
    {
        return m_savePath;
    }

}


