#include "Container.h"
#include "Constant.h"
#include "RssTable.h"
#include "TransparentScrollArea.h"
#include <QColor>
#include <QGridLayout>
#include <QResizeEvent>
#include <QWidget>

Container::Container(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    initUI();
}

void Container::initUI() {
    rssTable = new RssTable(this);
    scrollArea = new TransparentScrollArea();
    layout = new QGridLayout(this);

    // 设置布局
    layout->addWidget(scrollArea, 0, 0);
    layout->setContentsMargins(3 * _borderWidth, 2 * _borderWidth, 3 * _borderWidth, 3 * _borderWidth);
    layout->setSpacing(0);
    scrollArea->setWidget(rssTable);
    scrollArea->setWidgetResizable(true);
    setLayout(layout);

    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
}

void Container::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (rssTable) {
        rssTable->setFixedWidth(this->width() - 6 * _borderWidth);
    }
}