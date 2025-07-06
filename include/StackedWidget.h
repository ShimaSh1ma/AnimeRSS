#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStackedWidget>

class StackedWidget : public QStackedWidget {
  public:
    explicit StackedWidget();
    ~StackedWidget() = default;

    StackedWidget(const StackedWidget&) = delete;
    StackedWidget& operator=(const StackedWidget&) = delete;
    StackedWidget(StackedWidget&&) = delete;
    StackedWidget& operator=(const StackedWidget&&) = delete;

    void switchWidget(int _index);

  private:
    QPropertyAnimation* posAnimation;
    QPropertyAnimation* opacityAnimation;
    QGraphicsOpacityEffect* opacityEffect;
    std::unique_ptr<QParallelAnimationGroup> switchAnimeGroup;
    void setWidget();

    size_t index;
};