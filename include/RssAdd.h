#pragma once

#include <QWidget>
#include <functional>

class QLabel;
class QVBoxLayout;
class QColor;

class RssAdd : public QWidget {
  public:
    explicit RssAdd(QWidget* parent = nullptr);
    ~RssAdd() = default;

    std::function<void()> onAddClicked;

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    QLabel* addNormal;
    QLabel* addHover;
    QVBoxLayout* layout;
    void initUI(); // 初始化 UI

    QColor bgColor;

    bool isAddDialogOpen = false;
};