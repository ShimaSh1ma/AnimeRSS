#include "TitleBar.h"
#include "IconButton.h"
#include <QHBoxLayout>

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false); // 确保 TitleBar 接收鼠标事件
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
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
