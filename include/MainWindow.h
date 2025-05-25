#pragma once

#include <IconButton.h>
#include <QLayout>
#include <QWidget>

#include "TitleBar.h"

class MainWindow : public QWidget {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

  protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

  private:
    QVBoxLayout* mainLayout;
    void initUI();

    TitleBar* titleBar;
    void initTitleBar();

    QImage image;
    QPixmap pix;
    QPixmap temp;
    void stretchImage();
    int imageWidth = 0;
    int imageHeight = 0;
    int wWidth = 0;
    int wHeight = 0;
    int xpos = 0;
    int ypos = 0;
};
