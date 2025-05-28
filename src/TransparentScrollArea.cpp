#include "TransparentScrollArea.h"
#include "Constant.h"
#include <QScrollBar>

TransparentScrollArea::TransparentScrollArea() {
    setWindowFlags(Qt::FramelessWindowHint); // 窗口无边框化
    setFrameStyle(Qt::FramelessWindowHint);

    // 初始化滚动动画
    scrollAnimation = std::make_unique<QPropertyAnimation>(this->verticalScrollBar(), "value");
    scrollAnimation->setDuration(200);
    scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 窗口透明化
    QPalette pal = this->viewport()->palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    this->viewport()->setPalette(pal);

    setContentsMargins(0, 0, 0, 0);

    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TransparentScrollArea::wheelEvent(QWheelEvent* wheelEvent) {
    if (wheelEvent->source() == Qt::MouseEventNotSynthesized) {
        scrollAnimation->stop();
        scrollAnimation->setEndValue(this->verticalScrollBar()->value() - wheelEvent->angleDelta().y());
        scrollAnimation->start();
        wheelEvent->accept();
    } else {
        QScrollArea::wheelEvent(wheelEvent);
    }
}

void TransparentScrollArea::keyPressEvent(QKeyEvent* ev) {
    scrollAnimation->stop();
    int changeValue = 0;
    if (ev->key() == Qt::Key_Up) {
        changeValue = static_cast<int>(sizeScale(100));
    } else if (ev->key() == Qt::Key_Down) {
        changeValue = -static_cast<int>(sizeScale(100));
    } else if (ev->key() == Qt::Key_PageUp) {
        changeValue = static_cast<int>(sizeScale(100 + 5));
    } else if (ev->key() == Qt::Key_PageDown) {
        changeValue = -static_cast<int>(sizeScale(100 - 5));
    } else {
    }

    scrollAnimation->setEndValue(this->verticalScrollBar()->value() - changeValue);
    scrollAnimation->start();
}
