#include "Constant.h"

#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>

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