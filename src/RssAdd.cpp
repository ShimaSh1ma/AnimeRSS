#include "RssAdd.h"
#include "AddDialog.h"
#include "Constant.h"
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

inline QColor normalColor = QColor(80, 80, 80, _chosenOpacity * 255);
inline QColor hoverColor = QColor(80, 80, 80, _unchosenOpacity * 255);

RssAdd::RssAdd(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(_rssItemWidthMin);
    setMaximumWidth(_rssItemWidthMax);
    setFixedHeight(_rssItemHeight);
    initUI();
}

void RssAdd::paintEvent(QPaintEvent* event) {

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 背景路径
    QPainterPath path;
    path.addRoundedRect(this->rect(), _cornerRadius, _cornerRadius);
    painter.fillPath(path, bgColor);
}

void RssAdd::initUI() {
    addNormal = new QLabel();
    addHover = new QLabel();

    addNormal->setPixmap(QIcon(":/icons/add_normal").pixmap(sizeScale(100), sizeScale(100)));
    addHover->setPixmap(QIcon(":/icons/add_hover").pixmap(sizeScale(100), sizeScale(100)));

    addNormal->setAlignment(Qt::AlignCenter);
    addHover->setAlignment(Qt::AlignCenter);

    addNormal->setFixedSize(sizeScale(100), sizeScale(100));
    addHover->setFixedSize(sizeScale(100), sizeScale(100));

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(addNormal);
    layout->addWidget(addHover);
    addHover->hide();

    setLayout(layout);

    setCursor(Qt::PointingHandCursor);
    bgColor = normalColor;
}

void RssAdd::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed = true;
        mouseLeftWhilePressed = false;
        grabMouse();
        update();
    }
}

void RssAdd::mouseMoveEvent(QMouseEvent* event) {
    if (mousePressed) {
        bool inside = rect().contains(event->pos());
        if (!inside && !mouseLeftWhilePressed) {
            mouseLeftWhilePressed = true;
            addNormal->show();
            addHover->hide();
            bgColor = normalColor;
            update();
        } else if (inside && mouseLeftWhilePressed) {
            mouseLeftWhilePressed = false;
            addNormal->hide();
            addHover->show();
            bgColor = hoverColor;
            update();
        }
    }
}

void RssAdd::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        releaseMouse();
        if (rect().contains(event->pos()) && onAddClicked) {
            onAddClicked();
        }
        mousePressed = false;
        mouseLeftWhilePressed = false;
        update();
    }
}

void RssAdd::enterEvent(QEvent* event) {
    if (!mousePressed) {
        addNormal->hide();
        addHover->show();
        bgColor = hoverColor;
        update();
    }
}

void RssAdd::leaveEvent(QEvent* event) {
    mousePressed = false;
    addNormal->show();
    addHover->hide();
    bgColor = normalColor;
    update();
}