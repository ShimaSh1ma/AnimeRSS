#pragma once

#include <QTimer>
#include <QWidget>
#include <memory>
#include <shared_mutex>
#include <vector>

class QGridLayout;
class RssData;
class RssItem;
class AddDialog;

class RssTable : public QWidget {
  public:
    explicit RssTable(QWidget* parent = nullptr);
    ~RssTable() = default;

  protected:
    void resizeEvent(QResizeEvent* event) override;

  private:
    void initRssAdd();

    std::unique_ptr<QTimer> rssTimer;

    std::vector<std::shared_ptr<RssData>> rssList;
    std::shared_mutex rssListMutex;

    std::vector<std::unique_ptr<QWidget>> rssItems;
    AddDialog* addDialog = nullptr;

    QGridLayout* layout;
    void initLayout();

    void loadRssDatas();
    void initRssItems();
    void requestAllRss();
    void init();

    void clearLayout();
    void caculateColumn();
    int column = 0;
    void adjustLayout();

    void addRssData(const char* rssUrl, const char* savePath, const char* title);
    void deleteRssData(const RssItem* item);
    void openRssAddDialog();
};