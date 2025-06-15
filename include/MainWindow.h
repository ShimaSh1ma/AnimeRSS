#pragma once

#include <IconButton.h>
#include <QCloseEvent>
#include <QLayout>
#include <QStackedLayout>
#include <QSystemTrayIcon>
#include <QWidget>

class BackImg;
class TitleBar;
class Container;
class SettingWidget;
class StackedWidget;

class MainWindow : public QWidget {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

  protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

  private:
    StackedWidget* stackedWidget;
    void initStackedWidget();

    void createTrayIcon();
    QSystemTrayIcon* trayIcon;

    QVBoxLayout* mainLayout;
    void initUI();

    TitleBar* titleBar;
    void initTitleBar();

    Container* container;
    void initContainer();

    SettingWidget* settingWidget;
    void initSettingWidget();

    BackImg* backImg;
    void initBackImg();
};
