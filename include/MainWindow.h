#pragma once

#include <QWidget>

class MainWindow : public QWidget {
    Q_OBJECT
  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

  protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void paintEvent(QPaintEvent* event) override;
};
