#include "ShiftJISUtil.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <iconv.h>

#include <cstring>
#include <cerrno>

#include <vector>
#endif // defined(_WIN32)

#include "Logging.hpp"

std::string ShiftJISUtil::convertToUTF8(const char* string, const size_t stringSize) {
#if defined(_WIN32)
    const int wideLen = MultiByteToWideChar(932, 0, string, static_cast<int>(stringSize), nullptr, 0);
    if (wideLen == 0) {
        Logging::error("[ShiftJISUtil::convertToUTF8] MultiByteToWideChar failed: {}", GetLastError() );
        return "";
    }

    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(932, 0, string, static_cast<int>(stringSize), wideStr.data(), wideLen);

    const int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLen, nullptr, 0, nullptr, nullptr);
    if (utf8Len == 0) {
        Logging::error("[ShiftJISUtil::convertToUTF8] WideCharToMultiByte failed: {}", GetLastError());
        return "";
    }

    std::string utf8Str(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLen, utf8Str.data(), utf8Len, nullptr, nullptr);
    return utf8Str;
#else
    iconv_t convDesc = iconv_open("UTF-8", "SHIFT-JIS");
    if (convDesc == (iconv_t)-1) {
        Logging::error("[ShiftJISUtil::convertToUTF8] iconv_open failed: {}", std::strerror(errno));
        return "";
    }

    const size_t maxNewStringSize = stringSize * 4; // Worst-case scenario
    std::vector<char> newStringBuffer(maxNewStringSize);

    char* inputData = const_cast<char*>(string); // :(
    size_t inputDataLeft = stringSize;
    char* outputData = newStringBuffer.data();
    size_t outputDataLeft = maxNewStringSize;

    size_t convRes = iconv(convDesc, &inputData, &inputDataLeft, &outputData, &outputDataLeft);
    iconv_close(convDesc);

    if (convRes == (size_t)-1) {
        Logging::error("[ShiftJISUtil::convertToUTF8] iconv failed: {}", std::strerror(errno));
        return "";
    }

    return std::string(newStringBuffer.data(), maxNewStringSize - outputDataLeft);
#endif // defined(_WIN32)
}

std::string ShiftJISUtil::convertToShiftJIS(const char* string, const size_t stringSize) {
#if defined(_WIN32)
    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, string, static_cast<int>(stringSize), nullptr, 0);
    if (wideLen == 0) {
        Logging::error("[ShiftJISUtil::convertToShiftJIS] MultiByteToWideChar failed: {}", GetLastError());
        return "";
    }

    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, string, static_cast<int>(stringSize), wideStr.data(), wideLen);

    const int sjisLen = WideCharToMultiByte(932, 0, wideStr.c_str(), wideLen, nullptr, 0, nullptr, nullptr);
    if (sjisLen == 0) {
        Logging::error("[ShiftJISUtil::convertToShiftJIS] WideCharToMultiByte failed: {}", GetLastError());
        return "";
    }

    std::string sjisStr(sjisLen, 0);
    WideCharToMultiByte(932, 0, wideStr.c_str(), wideLen, sjisStr.data(), sjisLen, nullptr, nullptr);
    return sjisStr;
#else
    iconv_t convDesc = iconv_open("SHIFT-JIS", "UTF-8");
    if (convDesc == (iconv_t)-1) {
        Logging::error("[ShiftJISUtil::convertToShiftJIS] iconv_open failed: {}", std::strerror(errno));
        return "";
    }

    const size_t maxNewStringSize = stringSize * 2; // Worst-case scenario
    std::vector<char> newStringBuffer(maxNewStringSize);

    char* inputData = const_cast<char*>(string); // :(
    size_t inputDataLeft = stringSize;
    char* outputData = newStringBuffer.data();
    size_t outputDataLeft = maxNewStringSize;

    size_t convRes = iconv(convDesc, &inputData, &inputDataLeft, &outputData, &outputDataLeft);
    iconv_close(convDesc);

    if (convRes == (size_t)-1) {
        Logging::error("[ShiftJISUtil::convertToShiftJIS] iconv failed: {}", std::strerror(errno));
        return "";
    }

    return std::string(newStringBuffer.data(), maxNewStringSize - outputDataLeft);
#endif // defined(_WIN32)
}
