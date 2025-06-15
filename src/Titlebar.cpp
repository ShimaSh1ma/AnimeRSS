#include "TitleBar.h"
#include "Constant.h"
#include "IconButton.h"
#include <QApplication>
#include <QHBoxLayout>

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    settingButton = new IconButton();
    minimizeButton = new IconButton();
    closeButton = new IconButton();

    settingButton->setIcons(QIcon(":/icons/setting_normal"), QIcon(":/icons/setting_hover"));
    minimizeButton->setIcons(QIcon(":/icons/minimize_normal"), QIcon(":/icons/minimize_hover"));
    closeButton->setIcons(QIcon(":/icons/close_normal"), QIcon(":/icons/close_hover"));

    settingButton->setBackColor(Qt::transparent, Qt::gray);
    minimizeButton->setBackColor(Qt::transparent, Qt::gray);
    closeButton->setBackColor(Qt::transparent, Qt::red);

    layout->addStretch();
    layout->addWidget(settingButton);
    layout->addWidget(minimizeButton);
    layout->addWidget(closeButton);

    connect(settingButton, &IconButton::clicked, this, &TitleBar::settingClicked);
    connect(minimizeButton, &IconButton::clicked, this, &TitleBar::minimizeClicked);
    connect(closeButton, &IconButton::clicked, this, &TitleBar::closeClicked);

    setLayout(layout);
}

QRect TitleBar::getSettingButtonRect() const {
    return settingButton->geometry();
}

QRect TitleBar::getMinimizeButtonRect() const {
    return minimizeButton->geometry();
}

QRect TitleBar::getCloseButtonRect() const {
    return closeButton->geometry();
}

void TitleBar::settingClicked() {
    if (openSettingFunction) {
        openSettingFunction();
    }
}

void TitleBar::minimizeClicked() {
    if (parentWidget()) {
        parentWidget()->showMinimized();
    }
}

void TitleBar::closeClicked() {
    if (parentWidget()) {
        parentWidget()->close();
    }
}
