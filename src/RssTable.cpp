#include "RssTable.h"
#include "AddDialog.h"
#include "Constant.h"
#include "RssAdd.h"
#include "RssData.h"
#include "RssItem.h"
#include <QDebug>
#include <QGridLayout>
#include <filesystem>
#include <functional>

RssTable::RssTable(QWidget* parent) : QWidget(parent) {
    init();
}

void RssTable::init() {
    initUI();
    initRssAdd();
    loadRssDatas();
    initRssItems();
}

void RssTable::loadRssDatas() {
    namespace fs = std::filesystem;
    const std::string rssDir = ".\\rss";

    if (!fs::exists(rssDir) || !fs::is_directory(rssDir)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(rssDir)) {
        if (entry.is_regular_file()) {
            const auto& path = entry.path();
            if (path.extension() == ".json") {
                std::ifstream inFile(path);
                if (!inFile) {
                    continue;
                }

                std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

                auto rssData = std::make_unique<RssData>();
                rssData->loadFromJson(content);
                rssList.push_back(std::move(rssData));
            }
        }
    }
}

void RssTable::initUI() {
    layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    // layout->setVerticalSpacing(2 * _borderWidth);
    // layout->setHorizontalSpacing(4 * _borderWidth);
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
}

void RssTable::initRssAdd() {
    RssAdd* rssAdd = new RssAdd(this);
    rssAdd->onAddClicked = [this]() { openRssAddDialog(); };
    rssAdd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    rssItems.emplace_back(std::unique_ptr<RssAdd>(rssAdd));
}

void RssTable::initRssItems() {
    for (const auto& rssData : rssList) {
        rssItems.emplace_back(std::make_unique<RssItem>(rssData.get(), std::bind(&RssTable::deleteRssData, this, std::placeholders::_1), this));
    }
    requestAllRss();
}

void RssTable::requestAllRss() {
    for (const auto& rssData : rssList) {
        rssData->requestRss();
    }
}

void RssTable::openRssAddDialog() {
    AddDialog* addDialog = new AddDialog(this);
    addDialog->onAddClicked = [this](const char* rssUrl, const char* savePath, const char* title) { addRssData(rssUrl, savePath, title); };
    addDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    addDialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    addDialog->exec();
}

void RssTable::addRssData(const char* rssUrl, const char* savePath, const char* title) {
    auto newRssData = std::make_unique<RssData>(rssUrl, savePath, title);
    newRssData->requestRss();
    rssItems.emplace_back(std::make_unique<RssItem>(newRssData.get(), std::bind(&RssTable::deleteRssData, this, std::placeholders::_1), this));
    rssList.push_back(std::move(newRssData));

    adjustLayout();
}

void RssTable::deleteRssData(const RssItem* item) {
    for (size_t i = 0; i < rssItems.size(); ++i) {
        if (rssItems[i].get() == item) {
            // 从布局中移除该 widget
            layout->removeWidget(rssItems[i].get());
            rssItems[i]->setParent(nullptr);
            adjustLayout();

            // 删除对应的数据和 UI
            rssItems.erase(rssItems.begin() + i);
            rssList[i - 1]->deleteData();
            rssList.erase(rssList.begin() + i - 1);
            break;
        }
    }
}

void RssTable::caculateColumn() {
    // 当前列数对应的窗口长度区间
    int maxLength = (this->column + 1) * _rssItemWidthMin // 上限
                    + this->column * this->layout->horizontalSpacing();
    int minLength = this->column * _rssItemWidthMin // 下限
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