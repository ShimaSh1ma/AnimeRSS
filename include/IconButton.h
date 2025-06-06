#pragma once

#include <QPushButton>

class QIcon;
class QColor;

class IconButton : public QPushButton {
    Q_OBJECT

  public:
    explicit IconButton(QWidget* parent = nullptr);
    void setIcons(const QIcon& normal, const QIcon& hover);
    void setBackColor(const QColor& normal, const QColor& hover);

  protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

  private:
    QIcon normalIcon;
    QIcon hoverIcon;
    QIcon currentIcon;

    bool isHovered = false;

    QColor normalColor;
    QColor hoverColor;
    QColor pressColor;
};
