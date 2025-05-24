#include "MainWindow.h"
#include "Constant.h"
#include <QEvent>
#include <QPainter>
#include <QTimer>
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "Dwmapi.lib")

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(sizeScale(800), sizeScale(600));
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result) {
    MSG* msg = static_cast<MSG*>(message);

    WId wid = effectiveWinId();
    if (!wid) {
        return false;
    }

    HWND hWnd = reinterpret_cast<HWND>(wid);
    if (!IsWindow(hWnd)) {
        return false;
    }

    switch (msg->message) {
    case WM_NCCALCSIZE: {
        if (msg->wParam == TRUE) {
            if (windowState().testFlag(Qt::WindowMaximized)) {
                NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
                const int frameX = GetSystemMetrics(SM_CXFRAME);
                const int frameY = GetSystemMetrics(SM_CYFRAME);
                const int padding = GetSystemMetrics(SM_CXPADDEDBORDER);

                params->rgrc[0].left += frameX + padding;
                params->rgrc[0].top += frameY + padding;
                params->rgrc[0].right -= frameX + padding;
                params->rgrc[0].bottom -= frameY + padding;

                *result = 0;
                return true;
            }
        }
        return false;
    }

    case WM_NCHITTEST: {
        RECT winrect;
        GetWindowRect(hWnd, &winrect);
        const POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};

        const int resizeBorderX = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        const int resizeBorderY = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

        bool onLeft = pt.x >= winrect.left && pt.x < winrect.left + resizeBorderX;
        bool onRight = pt.x <= winrect.right && pt.x > winrect.right - resizeBorderX;
        bool onTop = pt.y >= winrect.top && pt.y < winrect.top + resizeBorderY;
        bool onBottom = pt.y <= winrect.bottom && pt.y > winrect.bottom - resizeBorderY;

        // 角部检测
        if (onTop && onLeft) {
            *result = HTTOPLEFT;
            return true;
        }
        if (onTop && onRight) {
            *result = HTTOPRIGHT;
            return true;
        }
        if (onBottom && onLeft) {
            *result = HTBOTTOMLEFT;
            return true;
        }
        if (onBottom && onRight) {
            *result = HTBOTTOMRIGHT;
            return true;
        }

        // 边部检测
        if (onLeft) {
            *result = HTLEFT;
            return true;
        }
        if (onRight) {
            *result = HTRIGHT;
            return true;
        }
        if (onTop) {
            *result = HTTOP;
            return true;
        }
        if (onBottom) {
            *result = HTBOTTOM;
            return true;
        }

        QPoint localPos = mapFromGlobal(QPoint(pt.x, pt.y));
        QRect titleBarRect(0, 0, width(), sizeScale(40));
        if (titleBarRect.contains(localPos)) {
            *result = HTCAPTION;
            return true;
        }
        break;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);

        RECT primaryWorkArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryWorkArea, 0);

        mmi->ptMaxPosition.x = -GetSystemMetrics(SM_CXFRAME);
        mmi->ptMaxPosition.y = -GetSystemMetrics(SM_CYFRAME);
        mmi->ptMaxSize.x = primaryWorkArea.right - primaryWorkArea.left + 2 * GetSystemMetrics(SM_CXFRAME);
        mmi->ptMaxSize.y = primaryWorkArea.bottom - primaryWorkArea.top + 2 * GetSystemMetrics(SM_CYFRAME);

        // 最小尺寸设置
        mmi->ptMinTrackSize.x = sizeScale(400);
        mmi->ptMinTrackSize.y = sizeScale(300);

        *result = 0;
        return true;
    }

    case WM_DPICHANGED: {
        const RECT* rect = reinterpret_cast<RECT*>(msg->lParam);
        SetWindowPos(hWnd, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        *result = 0;
        return true;
    }

    default:
        break;
    }

    return QWidget::nativeEvent(eventType, message, result);
}

void MainWindow::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect(), sizeScale(4), sizeScale(4)); // 绘制圆角矩形，圆角半径为10
    painter.setClipPath(path);                               // 设置绘制区域为圆角路径
    painter.fillRect(rect(), QColor(40, 40, 40, 255));       // 半透明灰色背景
}