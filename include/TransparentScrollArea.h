#pragma once

#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QWheelEvent>
#include <memory>

class TransparentScrollArea : public QScrollArea {
  public:
    explicit TransparentScrollArea();
    ~TransparentScrollArea() = default;

    TransparentScrollArea(const TransparentScrollArea&) = delete;
    TransparentScrollArea& operator=(const TransparentScrollArea&) = delete;
    TransparentScrollArea(TransparentScrollArea&&) = delete;
    TransparentScrollArea& operator=(const TransparentScrollArea&&) = delete;

  private:
    std::unique_ptr<QPropertyAnimation> scrollAnimation;

  protected:
    void wheelEvent(QWheelEvent* wheelEvent) override;
    void keyPressEvent(QKeyEvent* ev) override;
};