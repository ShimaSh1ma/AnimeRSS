#include "Iconbutton.h"
#include "Constant.h"
#include "Iconbutton.h"
#include <QColor>
#include <QDebug>
#include <QEvent>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>

IconButton::IconButton(QWidget* parent) : QPushButton(parent) {
    hoverColor = Qt::transparent;
    normalColor = Qt::transparent;

    setFlat(true);
    setCursor(Qt::PointingHandCursor);

    setIconSize(QSize(_iconSize, _iconSize));
    setFixedSize(_iconButtonSize, _iconButtonSize);

    setAttribute(Qt::WA_Hover); // 启用 hover 事件
}

void IconButton::setIcons(const QIcon& normal, const QIcon& hover) {
    normalIcon = normal;
    hoverIcon = hover;
    currentIcon = normalIcon;
    update();
}

void IconButton::setBackColor(const QColor& normal, const QColor& hover) {
    normalColor = normal;
    hoverColor = hover;
    update();
}

void IconButton::enterEvent(QEvent* event) {
    currentIcon = hoverIcon;
    isHovered = true;
    update();
    QPushButton::enterEvent(event);
}

void IconButton::leaveEvent(QEvent* event) {
    currentIcon = normalIcon;
    isHovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void IconButton::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景色
    QColor bgColor = isHovered ? hoverColor : normalColor;
    painter.fillRect(rect(), bgColor);

    // 图标绘制
    if (!currentIcon.isNull()) {
        QSize iconSize = this->iconSize();
        QRect iconRect((width() - iconSize.width()) / 2, (height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());

        currentIcon.paint(&painter, iconRect);
    }
}
