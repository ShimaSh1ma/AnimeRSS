#pragma once

#include <QWidget>

class QGridLayout;
class RssItem;
class RssTable;
class TransparentScrollArea;

class Container : public QWidget {
  public:
    explicit Container(QWidget* parent = nullptr);
    ~Container() = default;

  protected:
    void resizeEvent(QResizeEvent* event) override;

  private:
    RssTable* rssTable;                // RSS 表格
    TransparentScrollArea* scrollArea; // 滚动区域
    QGridLayout* layout;               // 布局管理器

    void initUI(); // 初始化 UI
};