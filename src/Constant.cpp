#include "Constant.h"
#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>
#include <filesystem>
#include <fstream>

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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void saveFile(const std::string& filePathStr, const std::string& data, std::ios_base::openmode mode) {
    try {
        std::filesystem::path filePath(filePathStr);
        std::filesystem::path directory = filePath.parent_path();

        // 创建目录（如果不存在）
        if (!directory.empty() && !std::filesystem::exists(directory)) {
            std::filesystem::create_directories(directory);
        }

        // 打开文件写入
        std::ofstream out(filePath, mode);
        if (!out) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return;
        }

        out << data;
        out.close(); // 可省略，RAII 会自动关闭
    } catch (const std::exception& ex) {
        std::cerr << "Exception in saveFile: " << ex.what() << std::endl;
    }
}
