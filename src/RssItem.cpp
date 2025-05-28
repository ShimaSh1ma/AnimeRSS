#include "RssItem.h"
#include "Codec.h"
#include "Constant.h"
#include "RssData.h"

#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

RssItem::RssItem(RssData* data, QWidget* parent) : QWidget(parent), rssData(data) {
    initUI();
    updateContent();
}

void RssItem::initUI() {
    // setMinimumWidth(_rssItemWidthMin);
    // setMaximumWidth(_rssItemWidthMax);
    setFixedHeight(_rssItemHeight); // 如果希望整个 RssItem 保持固定高度

    layout = new QVBoxLayout(this);
    layout->setContentsMargins(this->width() / 6, 0, this->width() / 6, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignCenter);

    title = new QLabel(this);
    title->setFont(QFont("SF Pro", sizeScale(14), QFont::Bold));
    // 仅设置基本样式，移除固定高度相关的属性
    title->setStyleSheet("QLabel {"
                         "   color: rgba(255,255,255,200);"
                         "   padding: 5px;"
                         "   background-color: transparent;"
                         "}");
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    // 使用 Preferred 高度，使得文本内容决定高度，同时宽度尽可能填满
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // 可选：如果需要最大显示三行并截断超长文字，则在 updateContent 中用 QFontMetrics::elidedText 自行处理
    layout->addWidget(title);
    setLayout(layout);
}

void RssItem::updateContent() {
    if (rssData) {
        // title = rssData->getTitle();
        // image = rssData->getImage();
    } else {
        title->setText(R"(时光流逝，饭菜依旧美味)");
        title->setToolTip(title->text()); // 设置工具提示
        image.load("D:/照片/动漫截图/时光流逝，饭菜依旧美味/无标题.png");
    }
    update();
}

void RssItem::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(this->rect(), _cornerRadius, _cornerRadius);
    painter.setClipPath(path);
    painter.fillRect(rect(), Qt::transparent);

    // 绘制图片，保持横纵比并填满
    if (!image.isNull()) {
        QRect targetRect = this->rect();
        QSize imageSize = image.size();
        imageSize.scale(targetRect.size(), Qt::KeepAspectRatioByExpanding);
        QRect filledRect(QPoint(0, 0), imageSize);
        filledRect.moveCenter(targetRect.center());
        painter.drawImage(filledRect, image);

        // 绘制半透明黑色蒙版
        painter.setOpacity(opacity);                           // 设置蒙版透明度
        painter.fillRect(filledRect, QColor(20, 20, 20, 255)); // 半透明黑色
    }
}

void RssItem::enterEvent(QEvent* event) {
    Q_UNUSED(event);
    // 鼠标进入事件处理
    qDebug("Mouse entered RssItem");
    opacity = _chosenOpacity; // 鼠标进入时设置为选中状态的透明度
    update();                 // 触发重绘以应用新的透明度
}

void RssItem::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    // 鼠标离开事件处理
    qDebug("Mouse left RssItem");
    opacity = _unchosenOpacity; // 鼠标离开时恢复为未选中状态的透明度
    update();                   // 触发重绘以应用新的透明度
}

void RssItem::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 处理鼠标按下事件
        qDebug("Mouse pressed on RssItem");
    }
}

void RssItem::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 处理鼠标释放事件
        qDebug("Mouse released on RssItem");
    }
}