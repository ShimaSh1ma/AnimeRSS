#pragma once

#include <QIcon>
#include <QPushButton>

class IconButton : public QPushButton {
    Q_OBJECT

  public:
    explicit IconButton(QWidget* parent = nullptr);
    void setIcons(const QIcon& normal, const QIcon& hover);

  protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    QIcon normalIcon; // 正常状态图标
    QIcon hoverIcon;  // 悬停状态图标
};
