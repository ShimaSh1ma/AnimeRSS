#include "Constant.h"
#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

static double ResizeRate = 1.0;

void setResizeRate(double rate) {
    ResizeRate = rate;
}

void initResizeRate() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        setResizeRate(1.0);
        return;
    }
    qreal dpi = screen->logicalDotsPerInch();

    if (dpi >= 240)
        setResizeRate(2.0);
    else if (dpi >= 180)
        setResizeRate(1.5);
    else if (dpi >= 120)
        setResizeRate(1.25);
    else
        setResizeRate(1.0);
}

double sizeScale(int size) {
    return size * ResizeRate;
}

std::wstring utf8_to_utf16(const std::string& utf8_str) {
    if (utf8_str.empty())
        return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), nullptr, 0);
    std::wstring utf16_str(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), &utf16_str[0], size_needed);
    return utf16_str;
}

void saveFile(const std::string& filePathStr, const std::string& data, std::ios_base::openmode mode) {
    try {
        std::wstring widePath = utf8_to_utf16(filePathStr);
        std::filesystem::path filePath(widePath);

        std::filesystem::path directory = filePath.parent_path();

        if (!directory.empty() && !std::filesystem::exists(directory)) {
            std::filesystem::create_directories(directory);
        }

        std::ofstream out;
        out.open(filePath.wstring(), mode);

        if (!out) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return;
        }

        out << data;
    } catch (const std::exception& ex) {
        std::cerr << "Exception in saveFile: " << ex.what() << std::endl;
    }
}
