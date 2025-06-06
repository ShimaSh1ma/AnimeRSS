#pragma once

#include <QWidget>
#include <memory>
#include <vector>

class QGridLayout;
class RssData;
class RssItem;

class RssTable : public QWidget {
  public:
    explicit RssTable(QWidget* parent = nullptr);
    ~RssTable() = default;

  protected:
    // void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    void initRssAdd();

    std::vector<std::unique_ptr<RssData>> rssList;
    std::vector<std::unique_ptr<QWidget>> rssItems;

    QGridLayout* layout;
    void initUI();

    void loadRssDatas();
    void initRssItems();
    void requestAllRss();
    void init();

    void caculateColumn();
    int column = 0; // 当前列数
    void adjustLayout();

    void addRssData(const char* rssUrl, const char* savePath, const char* title);
    void deleteRssData(const RssItem* item);
    void openRssAddDialog();
};