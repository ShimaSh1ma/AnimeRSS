#pragma once

#include <QImage>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QWidget>

inline double opacity = 0.2;

class BackImg : public QWidget {
    Q_OBJECT
  public:
    static BackImg* Instance(QWidget* parent = nullptr);

    void updateImg(const std::string& path);
    void cleanImg();
    void removeImg();

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
    Q_PROPERTY(qreal imgOpacity READ getImgOpacity WRITE setImgOpacity)
    explicit BackImg(QWidget* parent = nullptr);
    static BackImg* _instance;

    void stretchImage();
    void startFade(qreal from, qreal to);

    void fadeIn();
    void fadeOut();

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

    qreal imgOpacity = opacity;
};
