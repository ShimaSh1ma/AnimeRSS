#pragma once

#include <QDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <functional>

class AddDialog : public QDialog {
    Q_OBJECT

  public:
    explicit AddDialog(QWidget* parent = nullptr);
    ~AddDialog() = default;

    std::function<void(const char*, const char*, const char*)> onAddClicked;

    QString getTitle() const;
    QString getRssUrl() const;
    QString getSavePath() const;

  private slots:
    void selectSavePath();
    void validateInput();

  private:
    QLineEdit* titleInput;
    QLineEdit* rssUrlInput;
    QLineEdit* savePathInput;
    QPushButton* browseButton;
    QPushButton* addButton;
    QPushButton* cancelButton;
};
