#pragma once

#include <QDebug>
#include <QWidget>
#include <functional>

class QHBoxLayout;
class IconButton;

class TitleBar : public QWidget {
  public:
    explicit TitleBar(QWidget* parent = nullptr);
    ~TitleBar() = default;

    QRect getSettingButtonRect() const;
    QRect getMinimizeButtonRect() const;
    QRect getCloseButtonRect() const;

    std::function<void()> openSettingFunction;

  private:
    void settingClicked();
    void minimizeClicked();
    void closeClicked();
    QHBoxLayout* layout;
    IconButton* minimizeButton;
    IconButton* closeButton;
    IconButton* settingButton;
};
