#pragma once

#include "Constant.h"
#include <QWidget>

class QLabel;
class RssData;
class QVBoxLayout;

class RssItem : public QWidget {

  public:
    explicit RssItem(RssData* data, QWidget* parent = nullptr);
    ~RssItem() = default;

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    void initUI();
    void updateContent();

    QLabel* title;
    QImage image;
    RssData* rssData;

    QVBoxLayout* layout;

    double opacity = _unchosenOpacity;
};