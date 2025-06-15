#include "StackedWidget.h"
StackedWidget::StackedWidget() {
    // 初始化切换动画
    posAnimation = new QPropertyAnimation();
    opacityAnimation = new QPropertyAnimation();
    opacityEffect = new QGraphicsOpacityEffect();
    switchAnimeGroup = std::make_unique<QParallelAnimationGroup>();
    // 初始化索引
    index = 0;

    // 设置动画
    posAnimation->setDuration(150);
    posAnimation->setEasingCurve(QEasingCurve::InCubic);

    opacityAnimation->setDuration(150);
    opacityAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    // 切换动画完成切换窗口
    connect(this->switchAnimeGroup.get(), &QParallelAnimationGroup::finished, this, &StackedWidget::setWidget);
}

void StackedWidget::switchWidget(int _index) {
    if (_index == index) {
        return;
    }
    // 保存索引
    this->index = _index;
    // 绑定透明度遮罩
    this->currentWidget()->setGraphicsEffect(opacityEffect);
    // 绑定动画
    posAnimation->setTargetObject(this->currentWidget());
    posAnimation->setPropertyName("pos");

    opacityAnimation->setTargetObject(opacityEffect);
    opacityAnimation->setPropertyName("opacity");

    // 根据索引计算动画结束位置
    QPoint endValue{0, this->currentWidget()->height() / 2};
    if (this->index > _index) {
    } else {
        endValue = -endValue;
    }
    posAnimation->setStartValue(QPoint(0, 0));
    posAnimation->setEndValue(endValue);

    opacityAnimation->setStartValue(1.0);
    opacityAnimation->setEndValue(0.0);

    switchAnimeGroup->addAnimation(posAnimation);
    switchAnimeGroup->addAnimation(opacityAnimation);

    switchAnimeGroup->start();
}

void StackedWidget::setWidget() {
    this->setCurrentIndex(static_cast<int>(this->index));
}
