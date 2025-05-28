#pragma once

#include <QDebug>
#include <QWidget>

class QHBoxLayout;
class IconButton;

class TitleBar : public QWidget {
    Q_OBJECT

  public:
    explicit TitleBar(QWidget* parent = nullptr);
    ~TitleBar() = default;

    QRect getMinimizeButtonRect() const;
    QRect getCloseButtonRect() const;

  private:
    void minimizeClicked();
    void closeClicked();
    QHBoxLayout* layout;
    IconButton* minimizeButton;
    IconButton* closeButton;
};
