#include "MainWindow.h"
#include <QApplication>

#include "Constant.h"

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    initResizeRate();
    MainWindow window;
    window.show();
    return app.exec();
}