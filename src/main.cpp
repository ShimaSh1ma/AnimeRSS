#include "MainWindow.h"
#include <QApplication>

#include "Constant.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    initResizeRate();
    MainWindow window;
    window.show();
    return app.exec();
}