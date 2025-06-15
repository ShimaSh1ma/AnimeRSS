#pragma once

#include "Constant.h"
#include <QWidget>
#include <functional>
#include <memory>

class QLabel;
class RssData;
class QVBoxLayout;
class QPushButton;
class IconButton;

class RssItem : public QWidget {
    Q_OBJECT
  public:
    explicit RssItem(std::shared_ptr<RssData> data, std::function<void(RssItem*)> deleteFunction, QWidget* parent = nullptr);
    ~RssItem() = default;

  protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

  private:
    static void* chosenItem;

    bool mouseIn = false;

    void initUI();
    Q_INVOKABLE void updateContent();

    void replacePreview();

    std::function<void(RssItem*)> deleteFunction;

    QLabel* title;
    QImage image;
    std::weak_ptr<RssData> rssData;

    IconButton* deleteButton;
    IconButton* refreshButton;
    IconButton* previewButton;

    QVBoxLayout* layout;

    double opacity = _unchosenOpacity;
};