#include "RssTable.h"
#include "AddDialog.h"
#include "Constant.h"
#include "RssAdd.h"
#include "RssData.h"
#include "RssItem.h"
#include "RssRequestScheduler.h"
#include <QDebug>
#include <QGridLayout>
#include <QMessageBox>
#include <filesystem>
#include <functional>

RssTable::RssTable(QWidget* parent) : QWidget(parent) {
    init();
    rssTimer = std::make_unique<QTimer>();
    connect(rssTimer.get(), &QTimer::timeout, this, &RssTable::requestAllRss);
    rssTimer->start(10 * 60 * 1000);
}

void RssTable::init() {
    initLayout();
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

    std::vector<std::pair<fs::path, long long>> files;
    for (const auto& entry : fs::directory_iterator(rssDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string filename = entry.path().stem().string();
            auto pos = filename.rfind('_');
            if (pos != std::string::npos) {
                std::string ts = filename.substr(pos + 1);
                try {
                    long long timestamp = std::stoll(ts);
                    files.emplace_back(entry.path(), timestamp);
                } catch (...) {
                }
            }
        }
    }

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

    for (const auto& [path, _] : files) {
        std::ifstream inFile(path);
        if (!inFile)
            continue;
        std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        auto rssData = std::make_unique<RssData>();
        rssData->loadFromJson(content);
        rssList.push_back(std::move(rssData));
    }
}

void RssTable::initLayout() {
    layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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
    std::shared_lock lock(rssListMutex);
    for (const auto& rssData : rssList) {
        rssItems.emplace_back(std::make_unique<RssItem>(rssData, std::bind(&RssTable::deleteRssData, this, std::placeholders::_1), this));
    }
    requestAllRss();
}

void RssTable::requestAllRss() {
    std::shared_lock lock(rssListMutex);
    for (const auto& rssData : rssList) {
        RssRequestScheduler::instance().addTask(rssData);
    }
}

void RssTable::openRssAddDialog() {
    AddDialog* addDialog = new AddDialog(this);
    addDialog->onAddClicked = [this](const char* rssUrl, const char* savePath, const char* title) { addRssData(rssUrl, savePath, title); };
    addDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    addDialog->exec();
}

void RssTable::addRssData(const char* rssUrl, const char* savePath, const char* title) {
    auto newRssData = std::make_shared<RssData>(rssUrl, savePath, title);
    RssRequestScheduler::instance().addTask(newRssData);
    rssItems.emplace_back(std::make_unique<RssItem>(newRssData, std::bind(&RssTable::deleteRssData, this, std::placeholders::_1), this));
    std::unique_lock lock(rssListMutex);
    rssList.push_back(newRssData);
    adjustLayout();
}

void RssTable::deleteRssData(const RssItem* item) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Confirm"));
    msgBox.setText(tr("Removing this RSS subscription cannot be undone."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setIcon(QMessageBox::Question);

    msgBox.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    msgBox.setWindowIcon(QIcon());

    QMessageBox::StandardButton reply = static_cast<QMessageBox::StandardButton>(msgBox.exec());

    if (reply != QMessageBox::Yes) {
        return;
    }

    for (size_t i = 0; i < rssItems.size(); ++i) {
        if (rssItems[i].get() == item) {
            // 从布局中移除该 widget
            layout->removeWidget(rssItems[i].get());
            rssItems[i]->setParent(nullptr);
            rssItems.erase(rssItems.begin() + i);
            adjustLayout();

            std::thread([this, i]() {
                std::unique_lock lock(rssListMutex);
                rssList[i - 1]->deleteData();
                rssList.erase(rssList.begin() + i - 1);
            }).detach();
            break;
        }
    }
}

void RssTable::caculateColumn() {
    int maxLength = (this->column + 1) * _rssItemWidthMin + this->column * this->layout->horizontalSpacing();
    int minLength = this->column * _rssItemWidthMin + (this->column - 1) * this->layout->horizontalSpacing();

    if (this->width() < minLength || this->width() > maxLength) {
        int newColumn = this->size().width() / (_rssItemWidthMin + this->layout->horizontalSpacing());
        if (newColumn == 0) {
            newColumn = 1;
        }
        if (newColumn > static_cast<int>(rssItems.size())) {
            newColumn = static_cast<int>(rssItems.size());
        }

        if (newColumn != this->column) {
            this->column = newColumn;
            adjustLayout();
        }
    }
}

void RssTable::clearLayout() {
    if (!layout)
        return;
    delete layout;
    initLayout();
}

void RssTable::adjustLayout() {
    clearLayout();

    for (int c = 0; c < this->column; c++) {
        layout->setColumnStretch(c, 1);
    }

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