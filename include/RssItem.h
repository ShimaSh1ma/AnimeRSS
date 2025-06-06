#pragma once

#include "Constant.h"
#include <QWidget>
#include <functional>

class QLabel;
class RssData;
class QVBoxLayout;
class QPushButton;
class IconButton;

class RssItem : public QWidget {

  public:
    explicit RssItem(RssData* data, std::function<void(RssItem*)> deleteFunction, QWidget* parent = nullptr);
    ~RssItem() = default;

  protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    static void* chosenItem;

    bool mouseIn = false;

    void initUI();
    void updateContent();

    std::function<void(RssItem*)> deleteFunction;

    QLabel* title;
    QImage image;
    RssData* rssData;

    IconButton* deleteButton;
    IconButton* refreshButton;

    QVBoxLayout* layout;

    double opacity = _unchosenOpacity;
};