#pragma once

#include <QDebug>
#include <QWidget>

class QHBoxLayout;
class IconButton;

class TitleBar : public QWidget {
    Q_OBJECT

  public:
    explicit TitleBar(QWidget* parent = nullptr);

    QRect getMinimizeButtonRect() const;
    QRect getCloseButtonRect() const;

  signals:
    void minimizeClicked();
    void closeClicked();

  protected:
    void TitleBar::mousePressEvent(QMouseEvent* event) {
        qDebug() << "Mouse pressed on TitleBar";
        QWidget::mousePressEvent(event);
    }

    void TitleBar::mouseReleaseEvent(QMouseEvent* event) {
        qDebug() << "Mouse released on TitleBar";
        QWidget::mouseReleaseEvent(event);
    }

  private:
    QHBoxLayout* layout;
    IconButton* minimizeButton;
    IconButton* closeButton;
};
