#include "RssItem.h"
#include "BackImg.h"
#include "Codec.h"
#include "Constant.h"
#include "IconButton.h"
#include "RssData.h"

#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

void* RssItem::chosenItem = nullptr;

RssItem::RssItem(RssData* data, std::function<void(RssItem*)> deleteFunction, QWidget* parent)
    : QWidget(parent), deleteFunction(deleteFunction), rssData(data) {
    initUI();
    updateContent();
    data->updateUI = std::bind(&RssItem::updateContent, this);
}

void RssItem::initUI() {
    setMinimumWidth(_rssItemWidthMin);
    setMaximumWidth(_rssItemWidthMax);
    setFixedHeight(_rssItemHeight);

    deleteButton = new IconButton(this);
    deleteButton->setFixedSize(20, 20);
    deleteButton->setIcons(QIcon(":/icons/minimize_normal"), QIcon(":/icons/minimize_hover"));
    deleteButton->setBackColor(Qt::transparent, Qt::gray);
    deleteButton->move(width() - deleteButton->width() - sizeScale(6), sizeScale(6));

    deleteButton->raise();

    connect(deleteButton, &QPushButton::clicked, [this]() {
        if (deleteFunction) {
            deleteFunction(this);
        }
    });

    refreshButton = new IconButton(this);
    refreshButton->setFixedSize(20, 20);
    refreshButton->setIcons(QIcon(":/icons/minimize_normal"), QIcon(":/icons/minimize_hover"));
    refreshButton->setBackColor(Qt::transparent, Qt::gray);
    refreshButton->move(width() - 2 * (refreshButton->width() + sizeScale(6)), sizeScale(6));
    refreshButton->raise();

    connect(refreshButton, &QPushButton::clicked, [this]() { this->rssData->requestRss(); });

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(this->width() / 9, 0, this->width() / 9, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignCenter);

    title = new QLabel(this);
    title->setFont(QFont("SF Pro", sizeScale(16), QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QPalette palette = title->palette();
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255, 255));
    title->setPalette(palette);

    title->setAttribute(Qt::WA_TranslucentBackground);
    title->setAutoFillBackground(true);

    layout->addWidget(title);
    setLayout(layout);
}

void RssItem::updateContent() {
    if (rssData->getTitle() != "") {
        title->setText(rssData->getTitle().c_str());
        title->setToolTip(title->text());
    } else {
        title->setText("RSS");
        title->setToolTip(title->text());
    }
    if (rssData->getImage() != "") {
        image.load(rssData->getImage().c_str());
    } else {
        image = QImage(QSize(this->width(), this->height()), QImage::Format_RGB32);
        image.fill(Qt::gray);
    }

    update();
}

void RssItem::paintEvent(QPaintEvent* event) {
    if (!mouseIn) {
        if (RssItem::chosenItem == this) {
            opacity = _chosenOpacity;
        } else {
            opacity = _unchosenOpacity;
        }
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addRoundedRect(this->rect(), _cornerRadius, _cornerRadius);
    painter.setClipPath(path);
    painter.fillRect(rect(), Qt::transparent);

    if (!image.isNull()) {
        QRect targetRect = this->rect();
        QSize imageSize = image.size();
        imageSize.scale(targetRect.size(), Qt::KeepAspectRatioByExpanding);
        QRect filledRect(QPoint(0, 0), imageSize);
        filledRect.moveCenter(targetRect.center());
        painter.drawImage(filledRect, image);

        // 绘制半透明黑色蒙版
        painter.setOpacity(opacity);
        painter.fillRect(filledRect, QColor(20, 20, 20, 255));
    }
}

void RssItem::resizeEvent(QResizeEvent* event) {
    if (deleteButton) {
        deleteButton->move(width() - deleteButton->width() - sizeScale(6), sizeScale(6));
    }
    QWidget::resizeEvent(event);

    if (refreshButton) {
        refreshButton->move(width() - 2 * (refreshButton->width() + sizeScale(6)), sizeScale(6));
    }
    QWidget::resizeEvent(event);
}

void RssItem::enterEvent(QEvent* event) {
    mouseIn = true;
    opacity = _chosenOpacity;

    if (RssItem::chosenItem == nullptr) {
        if (rssData->getImage() != "") {
            BackImg::Instance()->updateImg(rssData->getImage());
        }
    }
    update();
}

void RssItem::leaveEvent(QEvent* event) {
    mouseIn = false;
    opacity = _unchosenOpacity;

    if (RssItem::chosenItem == nullptr) {
        BackImg::Instance()->cleanImg();
    }
    update();
}

void RssItem::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (RssItem::chosenItem == this) {
            RssItem::chosenItem = nullptr;
        } else {
            RssItem::chosenItem = this;
        }
    }
}
