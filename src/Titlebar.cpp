#include "TitleBar.h"
#include "Constant.h"
#include "IconButton.h"
#include <QApplication>
#include <QHBoxLayout>

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, _borderWidth, _borderWidth, 0);
    layout->setSpacing(sizeScale(4));
    layout->setAlignment(Qt::AlignRight);

    minimizeButton = new IconButton();
    closeButton = new IconButton();

    minimizeButton->setIcons(QIcon(":/icons/minimize_normal"), QIcon(":/icons/minimize_hover"));
    closeButton->setIcons(QIcon(":/icons/close_normal"), QIcon(":/icons/close_hover"));

    layout->addStretch();
    layout->addWidget(minimizeButton);
    layout->addWidget(closeButton);

    connect(minimizeButton, &IconButton::clicked, this, &TitleBar::minimizeClicked);
    connect(closeButton, &IconButton::clicked, this, &TitleBar::closeClicked);

    setLayout(layout);
}

QRect TitleBar::getMinimizeButtonRect() const {
    return minimizeButton->geometry(); // 返回最小化按钮的几何区域
}

QRect TitleBar::getCloseButtonRect() const {
    return closeButton->geometry(); // 返回关闭按钮的几何区域
}

void TitleBar::minimizeClicked() {
    if (parentWidget()) {
        parentWidget()->showMinimized(); // 最小化窗口
    }
}

void TitleBar::closeClicked() {
    if (parentWidget()) {
        parentWidget()->close(); // 关闭窗口
    }
}
