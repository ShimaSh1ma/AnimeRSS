#include <QWidget>

class BackImg : public QWidget {
  public:
    explicit BackImg(QWidget* parent = nullptr);
    ~BackImg() override = default;

  protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    QImage image;
    QPixmap pix;
    QPixmap temp;
    void stretchImage();
    int imageWidth = 0;
    int imageHeight = 0;
    int wWidth = 0;
    int wHeight = 0;
    int xpos = 0;
    int ypos = 0;
};