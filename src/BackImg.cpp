#include "BackImg.h"
#include "Constant.h"
#include <QGraphicsBlurEffect>
#include <QGraphicsOpacityEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <QWidget>
#include <filesystem>

BackImg* BackImg::_instance = nullptr;

BackImg* BackImg::Instance(QWidget* parent) {
    if (!_instance) {
        _instance = new BackImg(parent);
    }
    return _instance;
}

BackImg::BackImg(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);

    fadeAnim = new QPropertyAnimation(this, "imgOpacity", this);
    fadeAnim->setDuration(400);
    fadeAnim->setStartValue(0.2);
    fadeAnim->setEndValue(1.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void BackImg::updateImg(const std::string& path) {
    if (!std::filesystem::exists(utf8_to_utf16(path)))
        return;
    image.load(QString::fromUtf8(path.c_str()));
    pix = QPixmap::fromImage(image);
    stretchImage();
    update();
    fadeIn();
}

void BackImg::cleanImg() {
    fadeOut();
    update();
}

void BackImg::fadeIn() {
    startFade(opacity, 0.6);
}

void BackImg::fadeOut() {
    startFade(imgOpacity, 0.0);
}

void BackImg::removeImg() {
    image = QImage();
    pix = QPixmap();
    temp = QPixmap();
}

void BackImg::startFade(qreal from, qreal to) {
    fadeAnim->stop();
    fadeAnim->setStartValue(from);
    fadeAnim->setEndValue(to);
    fadeAnim->start();
}

void BackImg::stretchImage() {
    if (!pix.isNull()) {
        temp = pix.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        imageWidth = temp.width();
        imageHeight = temp.height();
        wWidth = this->width();
        wHeight = this->height();
        xpos = -(((imageWidth - wWidth) / 2 + (temp.width() - wWidth) / 2)) / 2;
        ypos = -(((imageHeight - wHeight) / 2 + (temp.height() - wHeight) / 2)) / 2;
    }
}

void BackImg::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    painter.fillRect(this->rect(), QColor(30, 30, 30));

    QRect titleBarRect(0, 0, this->width(), _titleBarHeight);
    QColor overlayColor(0, 0, 0, 100);
    painter.fillRect(titleBarRect, overlayColor);

    if (!pix.isNull()) {
        painter.setOpacity(imgOpacity * 0.5);
        painter.drawPixmap(xpos, ypos, temp);
    }
}

void BackImg::resizeEvent(QResizeEvent* event) {
    stretchImage();
    update();
}
