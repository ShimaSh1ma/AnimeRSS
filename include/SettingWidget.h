#pragma once

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>
#include <functional>

class IconButton;
class TitleLabel;
struct SettingConfig;

class SettingWidget : public QWidget {
  public:
    explicit SettingWidget(QWidget* parent = nullptr);

    void backToMain();

    std::function<void()> closeClicked;

  protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

  private:
    void loadFromGlobalConfig();
    void saveToGlobalConfig();

    IconButton* backButton;
    TitleLabel* title;
    QGridLayout* layout;

    QCheckBox* httpProxyCheck;
    QLineEdit* proxyAddressEdit;
    QSpinBox* proxyPortSpin;

    QLineEdit* qbitAddressEdit;
    QLineEdit* qbitUsernameEdit;
    QLineEdit* qbitPasswordEdit;

    QLineEdit* qbitIPEdit;
    QSpinBox* qbitPortSpin;

    QCheckBox* autoStartCheck;
    void initUI();
};
