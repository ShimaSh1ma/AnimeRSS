#include "Iconbutton.h"
#include "Constant.h"
#include <QDebug>
#include <QEvent>
#include <QFile>

IconButton::IconButton(QWidget* parent) : QPushButton(parent) {
    // 默认样式：无边框、透明背景
    setStyleSheet("border: none; background-color: transparent;");
    setIconSize(QSize(_iconSize, _iconSize));       // 图标大小
    setFixedSize(_iconButtonSize, _iconButtonSize); // 按钮大小
}

void IconButton::setIcons(const QIcon& normal, const QIcon& hover) {
    normalIcon = normal;
    hoverIcon = hover;
    setIcon(normalIcon); // 初始状态
}

void IconButton::enterEvent(QEvent* event) {
    setIcon(hoverIcon); // 悬停时切换图标
    QPushButton::enterEvent(event);
}

void IconButton::leaveEvent(QEvent* event) {
    setIcon(normalIcon); // 离开时恢复图标
    QPushButton::leaveEvent(event);
}