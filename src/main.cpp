#include "Constant.h"
#include "MainWindow.h"
#include "SocketModule/ClientSocket.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    WSAInit();
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    initResizeRate();
    MainWindow window;
    window.show();
    return app.exec();
}