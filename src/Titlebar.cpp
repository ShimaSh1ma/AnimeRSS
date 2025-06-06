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

    minimizeButton = new IconButton();
    closeButton = new IconButton();

    minimizeButton->setIcons(QIcon(":/icons/minimize_normal"), QIcon(":/icons/minimize_hover"));
    closeButton->setIcons(QIcon(":/icons/close_normal"), QIcon(":/icons/close_hover"));

    minimizeButton->setBackColor(Qt::transparent, Qt::gray);
    closeButton->setBackColor(Qt::transparent, Qt::red);

    layout->addStretch();
    layout->addWidget(minimizeButton);
    layout->addWidget(closeButton);

    connect(minimizeButton, &IconButton::clicked, this, &TitleBar::minimizeClicked);
    connect(closeButton, &IconButton::clicked, this, &TitleBar::closeClicked);

    setLayout(layout);
}

QRect TitleBar::getMinimizeButtonRect() const {
    return minimizeButton->geometry();
}

QRect TitleBar::getCloseButtonRect() const {
    return closeButton->geometry();
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
