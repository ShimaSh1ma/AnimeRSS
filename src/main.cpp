#include "Constant.h"
#include "MainWindow.h"
#include "SettingConfig.h"
#include "SocketModule/ClientSocket.h"
#include "WinToast.h"
#include <QApplication>
#include <filesystem>

int main(int argc, char* argv[]) {
    // 设置exe路径，保证开启启动时读取配置
    {
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::filesystem::path exePath(path);
        std::filesystem::current_path(exePath.parent_path());
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return -1;
    }
    WSAInit();
    initializeWinToast();
    GlobalConfig::config.loadFromFile();
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    initResizeRate();
    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/AnimeRss_main"));
    window.show();
    return app.exec();
}