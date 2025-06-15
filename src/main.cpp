#include "Constant.h"
#include "MainWindow.h"
#include "SettingConfig.h"
#include "SocketModule/ClientSocket.h"
#include "WinToast.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return -1;
    }
    WSAInit();
    initializeWinToast(L"AnimeRss");
    GlobalConfig::config.loadFromFile();
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    initResizeRate();
    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/AnimeRss_main"));
    window.show();
    return app.exec();
}