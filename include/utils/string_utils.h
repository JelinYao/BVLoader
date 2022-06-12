#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>
using std::string;
using std::wstring;
using std::vector;

namespace string_utils {

    //����ת��
    string UToA(const wstring& str);
    wstring AToU(const string& str);
    string UToUtf8(const wstring& wstrUnicode);
    wstring Utf8ToU(const string& str);
    string AToUtf8(const string& str);
    string Utf8ToA(const string& str);

    //�ַ���תСд
    // std::transform(str.begin(), str.end(), str.begin(), towlower);
    // std::transform(str.begin(), str.end(), str.begin(), towupper);

    //URL�����
    string UrlEncode(const string& strSrc);
    string UrlDecode(const string& strSrc);

    //����·���е��ļ���
    string GetNameByPathA(const string& strPath, bool bIsUrl);
    wstring GetNameByPathW(const wstring& strPath, bool bIsUrl);

    //�ַ����ָ�
    bool SplitStringW(const wstring& strSource, const wstring& strFlag, vector<wstring>& paramList);
    bool SplitStringA(const string& strSource, const string& strFlag, vector<string>& paramList);

    //�ַ����滻
    string StrReplaceA(const string& strContent, const string& strTag, const string& strReplace);
    wstring StrReplaceW(const wstring& strContent, const wstring& strTag, const wstring& strReplace);

    wstring TimeStampToString(__int64 time_stamp, bool millisecond);

    wstring FileSizeToString(__int64 size);

    wstring SecondsToString(int duration);

    wstring GetRandomText(int size);

    bool IsValidPhoneNumber(const std::wstring& phone_number);

    void StrTrim(std::string& str);

    std::string GetFileNameByUrl(const std::string& url);

    std::string GetFileExtentionNameByUrl(const std::string& url);

    // ȥ��Windows�ļ��������ܰ����������ַ���\/:*?"<>|
    template<class T>
    T TrimFileName(T ch)
    {
        if ((ch == '\\') || (ch == '/') || (ch == ':') || (ch == '*')
            || (ch == '?') || (ch == '"') || (ch == '<') || (ch == '>') || (ch == '|')) {
            return '#';
        }
        return ch;
    }

}