#include "MainWindow.h"
#include "Codec.h"
#include "Constant.h"
#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <QTimer>
#include <ShellScalingAPI.h>
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    resize(sizeScale(800), sizeScale(600));

    QString imagePath = QString::fromLocal8Bit("D:/照片/动漫截图/时光流逝，饭菜依旧美味/无标题.png");
    image.load(imagePath);
    pix = pix.fromImage(image);
    temp = pix;
    temp = pix.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    MainWindow::initUI();
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result) {
    MSG* msg = static_cast<MSG*>(message);

    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam) {
            *result = 0;
            return true; // Remove default window frame
        }
        break;

    case WM_NCHITTEST: {
        const LONG borderWidth = 8;
        RECT winRect;
        GetWindowRect(reinterpret_cast<HWND>(winId()), &winRect);

        const LONG x = GET_X_LPARAM(msg->lParam);
        const LONG y = GET_Y_LPARAM(msg->lParam);

        // 转换鼠标位置到窗口坐标系
        QPoint localPos = mapFromGlobal(QPoint(x, y));

        // 判断是否在 TitleBar 区域
        if (localPos.y() < _titleBarHeight) {
            // 判断是否在按钮区域
            if (titleBar) {
                QRect minimizeButtonRect = titleBar->getMinimizeButtonRect();
                QRect closeButtonRect = titleBar->getCloseButtonRect();

                if (minimizeButtonRect.contains(localPos) || closeButtonRect.contains(localPos)) {
                    *result = HTCLIENT; // 鼠标在按钮区域，事件传递到按钮
                    return true;
                }
            }

            *result = HTCAPTION; // 鼠标在 TitleBar 的非按钮区域，允许拖动或双击缩放
            return true;
        }

        // 处理窗口边框的拖动逻辑
        if (x >= winRect.left && x < winRect.left + borderWidth) {
            if (y >= winRect.top && y < winRect.top + borderWidth) {
                *result = HTTOPLEFT;
            } else if (y >= winRect.bottom - borderWidth && y < winRect.bottom) {
                *result = HTBOTTOMLEFT;
            } else {
                *result = HTLEFT;
            }
        } else if (x >= winRect.right - borderWidth && x < winRect.right) {
            if (y >= winRect.top && y < winRect.top + borderWidth) {
                *result = HTTOPRIGHT;
            } else if (y >= winRect.bottom - borderWidth && y < winRect.bottom) {
                *result = HTBOTTOMRIGHT;
            } else {
                *result = HTRIGHT;
            }
        } else if (y >= winRect.top && y < winRect.top + borderWidth) {
            *result = HTTOP;
        } else if (y >= winRect.bottom - borderWidth && y < winRect.bottom) {
            *result = HTBOTTOM;
        } else {
            *result = HTCLIENT; // 其他区域返回 HTCLIENT
        }
        return true;
    }

    case WM_LBUTTONDBLCLK: {
        QPoint globalPos = QCursor::pos();

        QPoint localPos = mapFromGlobal(globalPos);

        if (localPos.y() < _titleBarHeight) {
            if (titleBar) {
                QRect minimizeButtonRect = titleBar->getMinimizeButtonRect();
                QRect closeButtonRect = titleBar->getCloseButtonRect();

                if (!minimizeButtonRect.contains(localPos) && !closeButtonRect.contains(localPos)) {
                    if (IsZoomed(reinterpret_cast<HWND>(winId()))) {
                        ShowWindow(reinterpret_cast<HWND>(winId()), SW_RESTORE);
                    } else {
                        ShowWindow(reinterpret_cast<HWND>(winId()), SW_MAXIMIZE);
                    }
                    return true;
                }
            }
        }
        return false;
    }
    }

    return QWidget::nativeEvent(eventType, message, result);
}

void MainWindow::stretchImage() {
    if (!pix.isNull()) {
        // 保持横纵比缩放图片适应窗口大小
        temp = pix.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        this->imageWidth = temp.width();
        this->imageHeight = temp.height();
        wWidth = this->width();
        wHeight = this->height();
        xpos = -(((this->imageWidth - this->width()) / 2 + (temp.width() - this->wWidth) / 2)) / 2;
        ypos = -(((this->imageHeight - this->height()) / 2 + (temp.height() - this->wHeight) / 2)) / 2;
    }
}

void MainWindow::paintEvent(QPaintEvent* paintE) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(xpos, ypos, temp);

    QRect titleBarRect(0, 0, width(), height());
    QLinearGradient topGradient(titleBarRect.topLeft(), titleBarRect.bottomLeft());
    topGradient.setColorAt(0, QColor(255, 255, 255, 255));
    topGradient.setColorAt(1, QColor(255, 255, 255, 0));
    painter.fillRect(titleBarRect, topGradient);

    QRadialGradient edgeGradient(rect().center(), qMax(width(), height()) / 1.5, rect().center());
    edgeGradient.setColorAt(0, QColor(255, 255, 255, 0));
    edgeGradient.setColorAt(0.8, QColor(255, 255, 255, 50));
    edgeGradient.setColorAt(1, QColor(255, 255, 255, 150));
    painter.fillRect(rect(), edgeGradient);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    this->stretchImage();
}

void MainWindow::initUI() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignTop);

    initTitleBar();

    setLayout(mainLayout);
}

void MainWindow::initTitleBar() {
    titleBar = new TitleBar(this);
    titleBar->setFixedHeight(_titleBarHeight);
    mainLayout->addWidget(titleBar);
}
