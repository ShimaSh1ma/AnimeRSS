#include "MainWindow.h"
#include "BackImg.h"
#include "Codec.h"
#include "Constant.h"
#include "Container.h"
#include "TitleBar.h"
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QMenu>
#include <QPainter>
#include <QTextCodec>
#include <QTimer>
#include <ShellScalingAPI.h>
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);
    resize(sizeScale(800), sizeScale(600));

    MainWindow::initUI();
    MainWindow::createTrayIcon();
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result) {
    MSG* msg = static_cast<MSG*>(message);

    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam) {
            *result = 0;
            return true;
        }
        break;

    case WM_NCHITTEST: {
        RECT winRect;
        GetWindowRect(reinterpret_cast<HWND>(winId()), &winRect);

        const LONG x = GET_X_LPARAM(msg->lParam);
        const LONG y = GET_Y_LPARAM(msg->lParam);

        QPoint localPos = mapFromGlobal(QPoint(x, y));

        // 判断是否在 TitleBar 区域
        if (localPos.y() < _titleBarHeight + _borderWidth && localPos.y() >= _borderWidth &&
            localPos.x() >= _borderWidth && localPos.x() < this->width() - _borderWidth) {
            if (titleBar) {
                QRect minimizeButtonRect = titleBar->getMinimizeButtonRect();
                QRect closeButtonRect = titleBar->getCloseButtonRect();

                if (minimizeButtonRect.contains(localPos) || closeButtonRect.contains(localPos)) {
                    *result = HTCLIENT;
                    return true;
                }
            }

            *result = HTCAPTION;
            return true;
        }

        // 处理窗口边框的拖动逻辑
        if (x >= winRect.left && x < winRect.left + _borderWidth) {
            if (y >= winRect.top && y < winRect.top + _borderWidth) {
                *result = HTTOPLEFT;
            } else if (y >= winRect.bottom - _borderWidth && y < winRect.bottom) {
                *result = HTBOTTOMLEFT;
            } else {
                *result = HTLEFT;
            }
        } else if (x >= winRect.right - _borderWidth && x < winRect.right) {
            if (y >= winRect.top && y < winRect.top + _borderWidth) {
                *result = HTTOPRIGHT;
            } else if (y >= winRect.bottom - _borderWidth && y < winRect.bottom) {
                *result = HTBOTTOMRIGHT;
            } else {
                *result = HTRIGHT;
            }
        } else if (y >= winRect.top && y < winRect.top + _borderWidth) {
            *result = HTTOP;
        } else if (y >= winRect.bottom - _borderWidth && y < winRect.bottom) {
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

void MainWindow::createTrayIcon() {
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/close_normal"));
    trayIcon->setToolTip("AnimeRSS");

    QMenu* trayMenu = new QMenu(this);

    QAction* restoreAction = new QAction("Show", this);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::showNormal);

    QAction* quitAction = new QAction("Close", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    trayMenu->addAction(restoreAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            this->showNormal();
            this->activateWindow();
        }
    });
    trayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // if (trayIcon && trayIcon->isVisible()) {
    //     hide();
    //     event->ignore();
    // } else {
    //     event->accept();
    // }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (backImg) {
        backImg->resize(this->size());
    }
}

void MainWindow::paintEvent(QPaintEvent* paintE) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addRoundedRect(this->rect(), _cornerRadius, _cornerRadius);
    painter.setClipPath(path);
    painter.fillRect(this->rect(), QColor(0, 0, 0));
}

void MainWindow::initUI() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignTop);

    initBackImg();
    initTitleBar();
    initContainer();

    setLayout(mainLayout);
}

void MainWindow::initTitleBar() {
    titleBar = new TitleBar(this);
    titleBar->setFixedHeight(_titleBarHeight);
    mainLayout->addWidget(titleBar);
}

void MainWindow::initContainer() {
    container = new Container(this);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(container);
}

void MainWindow::initBackImg() {
    backImg = new BackImg(this);
    backImg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    backImg->show();
    backImg->stackUnder(mainLayout->parentWidget());
}
