#pragma once

#include <QTextCodec>
#include <string>
#include <windows.h>

inline const char* processChineseCodec(const std::string& path, const std::string& targetEncoding = "GB2312") {
#if defined(_WIN32)
    QTextCodec* codec = QTextCodec::codecForName(targetEncoding.c_str());
    static thread_local std::string buffer;
    buffer = codec->fromUnicode(QString::fromStdString(path)).toStdString();
    return buffer.c_str();
#else
    return path.c_str();
#endif
}
