#include "RssItem.h"
#include "BackImg.h"
#include "Constant.h"
#include "IconButton.h"
#include "RssData.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <filesystem>

constexpr int buttonSize = 30;

void* RssItem::chosenItem = nullptr;

RssItem::RssItem(std::shared_ptr<RssData> data, std::function<void(RssItem*)> deleteFunction, QWidget* parent)
    : QWidget(parent), deleteFunction(deleteFunction), rssData(data) {
    initUI();
    updateContent();

    std::weak_ptr<RssData> weakData = rssData;
    QPointer<RssItem> safeThis = this;
    data->updateUI = [safeThis, weakData]() {
        if (!safeThis)
            return;

        if (auto sp = weakData.lock()) {
            safeThis->updateContent();
        }
    };
}

void RssItem::initUI() {
    setMinimumWidth(_rssItemWidthMin);
    setMaximumWidth(_rssItemWidthMax);
    setFixedHeight(_rssItemHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    deleteButton = new IconButton(this);
    deleteButton->setFixedSize(sizeScale(buttonSize), sizeScale(buttonSize));
    deleteButton->setIcons(QIcon(":/icons/delete_normal"), QIcon(":/icons/delete_hover"));
    deleteButton->setBackColor(Qt::transparent, Qt::transparent);
    deleteButton->setToolTip("delete");
    deleteButton->raise();

    connect(deleteButton, &QPushButton::clicked, [this]() {
        if (deleteFunction) {
            deleteFunction(this);
        }
    });

    refreshButton = new IconButton(this);
    refreshButton->setFixedSize(sizeScale(buttonSize), sizeScale(buttonSize));
    refreshButton->setIcons(QIcon(":/icons/refresh_normal"), QIcon(":/icons/refresh_hover"));
    refreshButton->setBackColor(Qt::transparent, Qt::transparent);
    refreshButton->setToolTip("refresh");
    refreshButton->raise();

    connect(refreshButton, &QPushButton::clicked, [this]() {
        if (auto sp = rssData.lock()) {
            sp->requestRss();
        }
    });

    previewButton = new IconButton(this);
    previewButton->setFixedSize(sizeScale(buttonSize), sizeScale(buttonSize));
    previewButton->setIcons(QIcon(":/icons/image_normal"), QIcon(":/icons/image_hover"));
    previewButton->setBackColor(Qt::transparent, Qt::transparent);
    previewButton->setToolTip("replace preview");
    previewButton->raise();

    connect(previewButton, &QPushButton::clicked, [this]() { this->replacePreview(); });

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(this->width() / 9, 0, this->width() / 9, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignCenter);

    title = new QLabel(this);
    title->setFont(QFont("Noto Sans", sizeScale(16), QFont::Bold));
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

    setCursor(Qt::PointingHandCursor);
}

void RssItem::updateContent() {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "updateContent", Qt::QueuedConnection);
        return;
    }

    auto sp = rssData.lock();
    if (!sp)
        return;

    if (sp->getTitle() != "") {
        title->setText(sp->getTitle().c_str());
        title->setToolTip(title->text());
    } else {
        title->setText("RSS");
        title->setToolTip(title->text());
    }

    if (std::filesystem::exists(utf8_to_utf16(sp->getImage()))) {
        image.load(QString::fromUtf8(sp->getImage().c_str()));
    } else {
        image = QImage(QSize(this->width(), this->height()), QImage::Format_RGB32);
        image.fill(Qt::gray);
    }
    update();
}

void RssItem::replacePreview() {
    auto sp = rssData.lock();
    if (!sp)
        return;

    std::string filePath = QFileDialog::getOpenFileName(nullptr, tr("Replace Preview"), "", tr("Preview (*.png *.jpg *.jpeg *.bmp *.webp)")).toStdString();
    sp->setImage(filePath);
    updateContent();
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

        painter.setOpacity(opacity);
        painter.fillRect(filledRect, QColor(20, 20, 20, 255));
    }

    if (RssItem::chosenItem == this) {
        QPen pen(QColor(255, 255, 255, 255)); // 白色边框，带透明度
        pen.setWidth(sizeScale(4));
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QRectF borderRect = this->rect().adjusted(1, 1, -1, -1);
        painter.drawRoundedRect(borderRect, _cornerRadius, _cornerRadius);
    }

    if (auto sp = rssData.lock()) {
        if (!sp->getRead()) {
            int dotRadius = sizeScale(6);
            QPoint dotCenter = QPoint(sizeScale(9), sizeScale(9));
            painter.setOpacity(1.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 36, 32, 255));
            painter.drawEllipse(dotCenter, dotRadius, dotRadius);
        }
    }
}

void RssItem::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    int buttonIndex = 0;
    int spacing = sizeScale(6);
    int buttonWidth = sizeScale(buttonSize);
    auto nextButtonX = [&]() { return width() - (++buttonIndex) * (buttonWidth + spacing); };

    if (deleteButton) {
        deleteButton->move(nextButtonX(), spacing);
    }

    if (refreshButton) {
        refreshButton->move(nextButtonX(), spacing);
    }

    if (previewButton) {
        previewButton->move(nextButtonX(), spacing);
    }

    if (title) {
        QFontMetrics fm(title->font());
        int width = title->width();
        QString text = title->text();
        QRect rect = fm.boundingRect(0, 0, width, 1000, Qt::TextWordWrap, text);
        title->setFixedHeight(rect.height());
    }
}

void RssItem::enterEvent(QEvent* event) {
    mouseIn = true;
    opacity = _chosenOpacity;

    title->setVisible(false);
    if (RssItem::chosenItem == nullptr) {
        if (auto sp = rssData.lock()) {
            if (std::filesystem::exists(utf8_to_utf16(sp->getImage()))) {
                BackImg::Instance()->updateImg(sp->getImage());
            }
        }
    }
    update();
}

void RssItem::leaveEvent(QEvent* event) {
    mouseIn = false;
    opacity = _unchosenOpacity;

    title->setVisible(true);
    if (RssItem::chosenItem == nullptr) {
        BackImg::Instance()->cleanImg();
    }
    update();
}

void RssItem::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        auto sp = rssData.lock();
        if (!sp)
            return;

        QString folderPath = QString::fromStdString(sp->getSavePath());
        if (QDir(folderPath).exists()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
            sp->setRead();
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}
