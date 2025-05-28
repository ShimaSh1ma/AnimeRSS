#include "BackImg.h"
#include "Constant.h"
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <QWidget>

BackImg::BackImg(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);

    QString imagePath = "D:/照片/动漫截图/时光流逝，饭菜依旧美味/无标题.png";
    image.load(imagePath);
    pix = pix.fromImage(image);
    temp = pix;
    temp = pix.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    stretchImage();
}

void BackImg::stretchImage() {
    if (!pix.isNull()) {
        // 保持横纵比缩放图片适应窗口大小
        temp = pix.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        this->imageWidth = temp.width();
        this->imageHeight = temp.height();
        wWidth = this->width();
        wHeight = this->height();
        xpos = -(((this->imageWidth - this->width()) / 2 + (temp.width() - this->wWidth) / 2)) / 2;
        ypos = -(((this->imageHeight - this->height()) / 2 + (temp.height() - this->wHeight) / 2)) / 2;
    }
}

void BackImg::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addRoundedRect(this->rect(), _cornerRadius, _cornerRadius);
    painter.setClipPath(path);
    painter.fillRect(this->rect(), Qt::transparent);

    // 绘制背景图片
    if (!pix.isNull()) {
        painter.setOpacity(0.5);
        painter.drawPixmap(xpos, ypos, temp);
    }

    // 绘制标题栏区域
    QRect titleBarRect(0, 0, this->width(), _titleBarHeight);
    QPixmap titleBarPixmap = temp.copy(titleBarRect);
    QColor overlayColor(0, 0, 0, 200); // 半透明黑色
    painter.fillRect(titleBarRect, overlayColor);
}

void BackImg::resizeEvent(QResizeEvent* event) {
    this->stretchImage();
}