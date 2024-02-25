#include "string_utils.h"
#include <windows.h>
#include <intsafe.h>
#include <time.h>
#include <regex>
#include <string>

namespace string_utils {

    string UToA(const wstring& str)
    {
        string strDes;
        char* pBuffer = NULL;
        int nLen = 0;
        if (str.empty())
            strDes;
        nLen = ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), NULL, 0, NULL, NULL);
        if (0 == nLen)
            strDes;
        pBuffer = new char[nLen + 1];
        memset(pBuffer, 0, nLen + 1);
        ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), pBuffer, nLen, NULL, NULL);
        pBuffer[nLen] = '\0';
        strDes.append(pBuffer);
        delete[] pBuffer;
        return strDes;
    }

    wstring AToU(const string& str)
    {
        wstring strDes;
        wchar_t* pBuffer = NULL;
        int nLen = 0;
        if (str.empty())
            return strDes;
        nLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
        if (0 == nLen)
            return strDes;
        pBuffer = new wchar_t[nLen + 1];
        memset(pBuffer, 0, nLen + 1);
        ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), pBuffer, nLen);
        pBuffer[nLen] = '\0';
        strDes.append(pBuffer);
        delete[] pBuffer;
        return strDes;
    }

    string UToUtf8(const wstring& wstrUnicode)
    {
        string strRet;
        if (wstrUnicode.empty())
            return strRet;
        int nLen = WideCharToMultiByte(CP_UTF8, 0, wstrUnicode.c_str(), -1, NULL, 0, NULL, NULL);
        char* pBuffer = new char[nLen + 1];
        pBuffer[nLen] = '\0';
        nLen = WideCharToMultiByte(CP_UTF8, 0, wstrUnicode.c_str(), -1, pBuffer, nLen, NULL, NULL);
        strRet.append(pBuffer);
        delete[] pBuffer;
        return strRet;
    }

    wstring Utf8ToU(const string& str)
    {
        int u16Len = ::MultiByteToWideChar(CP_UTF8, NULL, str.c_str(), (int)str.size(), NULL, 0);
        wchar_t* wstrBuf = new wchar_t[u16Len + 1];
        ::MultiByteToWideChar(CP_UTF8, NULL, str.c_str(), (int)str.size(), wstrBuf, u16Len);
        wstrBuf[u16Len] = L'\0';
        wstring wStr;
        wStr.assign(wstrBuf, u16Len);
        delete[] wstrBuf;
        return wStr;
    }

    string AToUtf8(const string& str)
    {
        wstring strUnicode = AToU(str);
        return UToUtf8(strUnicode);
    }

    string Utf8ToA(const string& str)
    {
        wstring strUnicode = Utf8ToU(str);
        return UToA(strUnicode);
    }

    bool SplitStringW(const wstring& strSource, const wstring& strFlag, vector<wstring>& paramList)
    {
        if (strSource.empty() || strFlag.empty())
            return false;
        paramList.clear();
        size_t nBeg = 0;
        size_t nFind = strSource.find(strFlag, nBeg);
        if (nFind == std::wstring::npos)
            paramList.push_back(strSource);
        else
        {
            while (true)
            {
                if (nFind != nBeg)
                    paramList.push_back(strSource.substr(nBeg, nFind - nBeg));
                nBeg = nFind + strFlag.size();
                if (nBeg == strSource.size())
                    break;
                nFind = strSource.find(strFlag, nBeg);
                if (nFind == std::wstring::npos)
                {
                    paramList.push_back(wstring(strSource.begin() + nBeg, strSource.end()));
                    break;
                }
            }
        }
        return true;
    }

    bool SplitStringA(const string& strSource, const string& strFlag, vector<string>& paramList)
    {
        if (strSource.empty() || strFlag.empty())
            return false;
        paramList.clear();
        size_t nBeg = 0;
        size_t nFind = strSource.find(strFlag, nBeg);
        if (nFind == std::string::npos)
            paramList.push_back(strSource);
        else
        {
            while (true)
            {
                if (nFind != nBeg)
                    paramList.push_back(strSource.substr(nBeg, nFind - nBeg));
                nBeg = nFind + strFlag.size();
                if (nBeg == strSource.size())
                    break;
                nFind = strSource.find(strFlag, nBeg);
                if (nFind == std::string::npos)
                {
                    paramList.push_back(string(strSource.begin() + nBeg, strSource.end()));
                    break;
                }
            }
        }
        return true;
    }

    inline	uint8_t ToHex(uint8_t ch)
    {
        return ch > 9 ? ch + 55 : ch + 48;
    }

    inline uint8_t FromHex(uint8_t ch)
    {
        uint8_t ret;
        if (ch > 'A' && ch < 'Z')
            ret = ch - 'A' + 10;
        else if (ch > 'a' && ch < 'z')
            ret = ch - 'a' + 10;
        else if (ch > '0' && ch < '9')
            ret = ch - '0';
        else ret = ch;
        return ret;
    }

    string UrlEncode(const string& strSrc)
    {
        string strDes;
        for (size_t i = 0; i < strSrc.size(); ++i)
        {
            uint8_t ch = (uint8_t)strSrc[i];
            if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
                strDes += ch;
            else if (ch == ' ')
                strDes += '+';
            else
            {
                strDes += '%';
                strDes += ToHex((ch >> 4));
                strDes += ToHex(ch % 16);
            }
        }
        return strDes;
    }

    string UrlDecode(const string& strSrc)
    {
        string strDes;
        for (size_t i = 0; i < strSrc.size(); i++)
        {
            uint8_t ch = strSrc[i];
            if (ch == '+')
                strDes += ' ';
            else if (ch == '%')
            {
                uint8_t h = FromHex((unsigned char)strSrc[++i]);
                uint8_t l = FromHex((unsigned char)strSrc[++i]);
                strDes += (h << 4) + l;
            }
            else strDes += ch;
        }
        return strDes;
    }

    string GetNameByPathA(const string& strPath, bool bIsUrl)
    {
        char ch = bIsUrl ? '/' : '\\';
        int nPos = strPath.find_last_of(ch);
        if (nPos == string::npos)
        {
            return "";
        }
        return string(strPath.begin() + nPos + 1, strPath.end());
    }

    wstring GetNameByPathW(const wstring& strPath, bool bIsUrl)
    {
        wchar_t ch = bIsUrl ? '/' : '\\';
        int nPos = strPath.find_last_of(ch);
        if (nPos == wstring::npos)
        {
            return L"";
        }
        return wstring(strPath.begin() + nPos + 1, strPath.end());
    }

    string StrReplaceA(const string& strContent, const string& strTag, const string& strReplace)
    {
        size_t nBegin = 0, nFind = 0;
        nFind = strContent.find(strTag, nBegin);
        if (nFind == string::npos)
            return strContent;
        size_t nTagLen = strTag.size();
        string strRet;
        while (true)
        {
            strRet.append(strContent.begin() + nBegin, strContent.begin() + nFind);
            strRet.append(strReplace);
            nBegin = nFind + nTagLen;
            nFind = strContent.find(strTag, nBegin);
            if (nFind == string::npos)
            {
                strRet.append(strContent.begin() + nBegin, strContent.end());
                break;
            }
        }
        return strRet;
    }

    wstring StrReplaceW(const wstring& strContent, const wstring& strTag, const wstring& strReplace)
    {
        size_t nBegin = 0, nFind = 0;
        nFind = strContent.find(strTag, nBegin);
        if (nFind == wstring::npos)
            return strContent;
        size_t nTagLen = strTag.size();
        wstring strRet;
        while (true)
        {
            strRet.append(strContent.begin() + nBegin, strContent.begin() + nFind);
            strRet.append(strReplace);
            nBegin = nFind + nTagLen;
            nFind = strContent.find(strTag, nBegin);
            if (nFind == wstring::npos)
            {
                strRet.append(strContent.begin() + nBegin, strContent.end());
                break;
            }
        }
        return strRet;
    }

    wstring TimeStampToString(__int64 time_stamp, bool millisecond)
    {
        if (millisecond) {
            time_stamp = time_stamp / 1000;
        }
        struct tm tt;
        wchar_t buffer[64] = { 0 };
        if (0 == localtime_s(&tt, &time_stamp)) {
            swprintf_s(buffer, L"%d-%02d-%02d", tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday);
        }
        return std::wstring(buffer);
    }

    static const int kGBSize = 1024 * 1024 * 1024;
    static const int kMBSize = 1024 * 1024;
    static const int kKBSize = 1024;

    wstring FileSizeToString(__int64 size)
    {
        double value = 0.0;
        wstring unit;
        if (size >= kGBSize) {
            value = (size >> 20) / 1024.0;
            unit = L"GB";
        }
        else if (size >= kMBSize) {
            value = (size >> 10) / 1024.0;
            unit = L"MB";
        }
        else if (size >= kKBSize) {
            value = size / 1024.0;
            unit = L"KB";
        }
        else {
            value = (double)size;
            unit = L"Bit";
        }
        wchar_t buffer[32];
        swprintf_s(buffer, L"%0.1f%s", value, unit.c_str());
        return std::wstring(buffer);
    }

    wstring SecondsToString(int duration)
    {
        int hours = duration / 3600;
        int remain = duration % 3600;
        int mins = remain / 60;
        remain = remain % 60;
        wchar_t buffer[16];
        swprintf_s(buffer, L"%02d:%02d:%02d", hours, mins, remain);
        return std::wstring(buffer);
    }

    wstring GetRandomText(int size)
    {
        wstring rand_text;
        if (size < 1) {
            return rand_text;
        }
        /*产生区间[a,z]上的随机数*/
        int left = size / 2;
        int right = size - left;

        srand((unsigned)time(NULL));
        for (int i = 0; i < left; ++i) {
            wchar_t v1 = (wchar_t)(rand() % 26) + 'a';
            rand_text += v1;
        }
        srand((unsigned)time(NULL));
        for (int i = 0; i < right; ++i) {
            wchar_t v1 = (wchar_t)(rand() % 26) + 'A';
            rand_text += v1;
        }
        return rand_text;
    }

    bool IsValidPhoneNumber(const std::wstring& phone_number)
    {
        std::wregex reg(L"^1[3|4|5|6|7|8|9][0-9]{9}$");
        return std::regex_match(phone_number.c_str(), reg);
    }

    void StrTrim(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !isspace(ch);
            }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !isspace(ch);
            }).base(), s.end());
    }

    std::string GetFileNameByUrl(const std::string& url)
    {
        std::string name;
        size_t pos = url.find("?");
        if (pos != std::string::npos) {
            name = url.substr(0, pos);
        }
        pos = url.rfind('/');
        if (pos != std::string::npos) {
            name = name.substr(pos + 1);
        }
        return name;
    }
    
    std::string GetFileExtentionNameByUrl(const std::string& url)
    {
        auto name = GetFileNameByUrl(url);
        if (name.empty()) {
            return "";
        }
        size_t pos = name.rfind('.');
        if (pos != std::string::npos) {
            return name.substr(pos);
        }
        return "";
    }

    std::map<std::wstring, std::wstring> CommandLineToArgMap(const wchar_t* command)
    {
        std::map<std::wstring, std::wstring> argMap;
        int argCount = 0;
        LPWSTR* cmdArray = ::CommandLineToArgvW(command, &argCount);
        if (cmdArray == nullptr || argCount == 0) {
            return argMap;
        }
        for (int i = 0; i < argCount; ++i) {
            std::wstring arg(cmdArray[i]);
            auto pos = arg.find('=');
            if (pos == std::wstring::npos) {
                continue;
            }
            std::wstring key = arg.substr(0, pos);
            std::wstring value = arg.substr(pos + 1);
            while (!key.empty() && key[0] == '-') {
                key.erase(key.begin());
            }
            argMap.emplace(std::move(key), std::move(value));
        }
        return argMap;
    }

}// namespace string_utils

