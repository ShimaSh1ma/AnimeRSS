#include "RssTable.h"
#include "Constant.h"
#include "RssData.h"
#include "RssItem.h"
#include <QDebug>
#include <QGridLayout>

RssTable::RssTable(QWidget* parent) : QWidget(parent) {
    init();
}

void RssTable::init() {
    initUI();
    loadRssDatas();
    createRssItems();
}

void RssTable::loadRssDatas() {
}

void RssTable::initUI() {
    layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setVerticalSpacing(2 * _borderWidth);
    layout->setHorizontalSpacing(4 * _borderWidth);
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
}

void RssTable::createRssItems() {
    for (const auto& rssData : rssDatas) {
        rssItems.emplace_back(std::make_unique<RssItem>(rssData.get(), this));
    }
    // 添加测试项
    for (int i = 0; i < 10; ++i) {
        rssItems.emplace_back(std::make_unique<RssItem>(nullptr, this));
    }
}

void RssTable::caculateColumn() {
    // 当前列数对应的窗口长度区间
    int maxLength = (this->column + 1) * _rssItemWidthMin // 上限
                    + this->column * this->layout->horizontalSpacing();
    int minLength = this->column * _rssItemWidthMax // 下限
                    + (this->column - 1) * this->layout->horizontalSpacing();

    // 超出区间则更新列数
    if (this->width() < minLength || this->width() > maxLength) {
        this->column = this->size().width() / (_rssItemWidthMin + this->layout->horizontalSpacing());
        if (this->column == 0) {
            this->column = 1;
        }
        adjustLayout();
    }
}

void RssTable::adjustLayout() {
    delete layout;
    initUI();

    int _row = 0, _column = 0;
    for (auto& it : this->rssItems) {
        if (_column < this->column) {
            this->layout->addWidget(it.get(), _row, _column, Qt::AlignTop);
            _column++;
        } else {
            _column = 0;
            _row++;
            this->layout->addWidget(it.get(), _row, _column, Qt::AlignTop);
            _column++;
        }
    }
}

void RssTable::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    caculateColumn();
}