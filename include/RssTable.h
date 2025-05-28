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
    std::vector<std::unique_ptr<RssData>> rssDatas;
    std::vector<std::unique_ptr<RssItem>> rssItems;

    QGridLayout* layout;
    void initUI();

    void loadRssDatas();
    void createRssItems();
    void init();

    void caculateColumn();
    int column = 0; // 当前列数
    void adjustLayout();
};