#pragma once

#include <QImage>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QWidget>

class BackImg : public QWidget {
    Q_OBJECT
  public:
    static BackImg* Instance(QWidget* parent = nullptr);

    void updateImg(const std::string& path);
    void cleanImg();
    void fadeIn();

    inline qreal getImgOpacity() const {
        return imgOpacity;
    }
    inline void setImgOpacity(qreal opacity) {
        imgOpacity = opacity;
        update();
    }

  protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    explicit BackImg(QWidget* parent = nullptr);
    void stretchImage();
    void startFade(qreal from, qreal to);

    static BackImg* _instance;

    QImage image;
    QPixmap pix;
    QPixmap temp;

    QPropertyAnimation* fadeAnim;

    int imageWidth = 0;
    int imageHeight = 0;
    int wWidth = 0;
    int wHeight = 0;
    int xpos = 0;
    int ypos = 0;

    Q_PROPERTY(qreal imgOpacity READ getImgOpacity WRITE setImgOpacity)
    qreal imgOpacity = 0.0;
};
