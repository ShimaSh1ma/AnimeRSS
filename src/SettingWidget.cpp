#include "SettingWidget.h"
#include "Constant.h"
#include "IconButton.h"
#include "SettingConfig.h"

#include <QCheckBox>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QSpinBox>
#include <string>
#include <windows.h>

bool setAutoRun(bool enable, const std::wstring& appName) {
    wchar_t path[MAX_PATH] = {0};
    DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return false;
    }

    std::wstring appPath = L"\"" + std::wstring(path) + L"\"";

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    if (enable) {
        // 设置开机自启
        if (RegSetValueExW(hKey, appName.c_str(), 0, REG_SZ, (const BYTE*)appPath.c_str(), (appPath.size() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return false;
        }
    } else {
        // 取消开机自启
        RegDeleteValueW(hKey, appName.c_str());
    }

    RegCloseKey(hKey);
    return true;
}

// 深色风格输入框（局部类）
class DarkLineEdit : public QLineEdit {
  public:
    explicit DarkLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {
        setStyleSheet("QLineEdit {"
                      "  background-color: #2E2E2E;"
                      "  color: #CCCCCC;"
                      "  border: 1px solid #555555;"
                      "  border-radius: 4px;"
                      "  padding: 4px 6px;"
                      "}"
                      "QLineEdit:focus {"
                      "  border: 1px solid #0078D7;"
                      "  background-color: #3A3A3A;"
                      "}");
    }
};

// 深色风格SpinBox，隐藏上下箭头（局部类）
class DarkSpinBox : public QSpinBox {
  public:
    explicit DarkSpinBox(QWidget* parent = nullptr) : QSpinBox(parent) {
        setStyleSheet("QSpinBox {"
                      "  background-color: #2E2E2E;"
                      "  color: #CCCCCC;"
                      "  border: 1px solid #555555;"
                      "  border-radius: 4px;"
                      "  padding-left: 6px;"
                      "}"
                      "QSpinBox::up-button, QSpinBox::down-button {"
                      "  height: 0; width: 0; border: none;"
                      "  background: transparent;"
                      "}");
    }
};

class TitleLabel : public QLabel {
  public:
    TitleLabel(const QString& text, const QColor& textColor = QColor(255, 255, 255), QWidget* parent = nullptr) : QLabel(text, parent) {
        setFont(QFont("SF Pro", sizeScale(10), QFont::Bold));
        setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        setWordWrap(true);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QPalette palette = this->palette();
        palette.setColor(QPalette::WindowText, textColor);
        setPalette(palette);
    }
};

class ToggleSwitch : public QCheckBox {
  public:
    explicit ToggleSwitch(QWidget* parent = nullptr) : QCheckBox(parent) {
        setFixedSize(sizeScale(48), sizeScale(24));
        setCursor(Qt::PointingHandCursor);
        setFont(QFont("SF Pro", sizeScale(12)));
        QPalette pal = palette();
        pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
        setPalette(pal);
        setChecked(false);
    }

  protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor bg = isChecked() ? QColor(255, 255, 255) : QColor(80, 80, 80);
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        QRectF rect(0, 0, width(), height());
        painter.drawRoundedRect(rect, height() / 2, height() / 2);

        QColor handleColor = !isChecked() ? QColor(255, 255, 255) : QColor(200, 200, 200);
        painter.setBrush(handleColor);
        painter.setPen(Qt::NoPen);

        int margin = sizeScale(3);
        int handleDiameter = height() - 2 * margin;

        qreal xPos = isChecked() ? width() - handleDiameter - margin : margin;
        QRectF handleRect(xPos, margin, handleDiameter, handleDiameter);
        painter.drawEllipse(handleRect);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            setChecked(!isChecked());
            update();
        }
        QCheckBox::mouseReleaseEvent(event);
    }
};

SettingWidget::SettingWidget(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    initUI();
    loadFromGlobalConfig();
}

void SettingWidget::initUI() {
    layout = new QGridLayout();
    layout->setSpacing(sizeScale(10));
    layout->setContentsMargins(4 * _borderWidth, 4 * _borderWidth, 8 * _borderWidth, 4 * _borderWidth);

    backButton = new IconButton();
    backButton->setIcons(QIcon(":/icons/back_normal"), QIcon(":/icons/back_hover"));
    backButton->setBackColor(Qt::transparent, Qt::transparent);
    connect(backButton, &QPushButton::clicked, this, &SettingWidget::backToMain);

    title = new TitleLabel("Settings");
    title->setFont(QFont("SF Pro", sizeScale(14), QFont::Bold));

    QHBoxLayout* titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(sizeScale(6));
    titleLayout->setMargin(0);
    titleLayout->addWidget(backButton);
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    layout->addLayout(titleLayout, 0, 0, 1, 3);

    int row = 1;
    int inputWidth = sizeScale(300);

    constexpr int tabWidth = 80;

    QColor titleColor = QColor(220, 220, 220);
    QColor subColor = QColor(180, 180, 180);

    QFont sectionFont = title->font();
    sectionFont.setPointSize(sizeScale(12));
    sectionFont.setBold(true);

    // 1. HTTP Proxy Enable Switch
    httpProxyCheck = new ToggleSwitch();

    TitleLabel* httpProxyLabel = new TitleLabel("HTTP Proxy", titleColor);
    httpProxyLabel->setContentsMargins(sizeScale(tabWidth / 2), 0, 0, 0);
    httpProxyLabel->setFont(sectionFont);
    layout->addWidget(httpProxyLabel, row, 0, Qt::AlignVCenter);
    layout->addWidget(httpProxyCheck, row++, 2, Qt::AlignRight);

    // Proxy Address
    TitleLabel* proxyAddressLabel = new TitleLabel("Proxy Address", subColor);
    proxyAddressLabel->setContentsMargins(sizeScale(tabWidth), 0, 0, 0);
    layout->addWidget(proxyAddressLabel, row, 0);
    proxyAddressEdit = new DarkLineEdit();
    proxyAddressEdit->setFixedWidth(inputWidth);
    layout->addWidget(proxyAddressEdit, row++, 1, 1, 2);

    // Proxy Port
    TitleLabel* proxyPortLabel = new TitleLabel("Proxy Port", subColor);
    proxyPortLabel->setContentsMargins(sizeScale(tabWidth), 0, 0, 0);
    layout->addWidget(proxyPortLabel, row, 0);
    proxyPortSpin = new DarkSpinBox();
    proxyPortSpin->setRange(1, 65535);
    proxyPortSpin->setValue(7890);
    proxyPortSpin->setFixedWidth(inputWidth);
    layout->addWidget(proxyPortSpin, row++, 1, 1, 2);

    // 2. qBittorrent section title
    QLabel* qbitSectionLabel = new TitleLabel("qBittorrent", titleColor);
    qbitSectionLabel->setContentsMargins(sizeScale(tabWidth / 2), 0, 0, 0);
    qbitSectionLabel->setFont(sectionFont);
    layout->addWidget(qbitSectionLabel, row++, 0, 1, 3);

    // qBittorrent IP
    TitleLabel* qbitIPLabel = new TitleLabel("IP", subColor);
    qbitIPLabel->setContentsMargins(sizeScale(tabWidth), 0, 0, 0);
    layout->addWidget(qbitIPLabel, row, 0);
    qbitIPEdit = new DarkLineEdit();
    qbitIPEdit->setText("127.0.0.1");
    qbitIPEdit->setFixedWidth(inputWidth);
    layout->addWidget(qbitIPEdit, row++, 1, 1, 2);

    // qBittorrent Port
    TitleLabel* qbitPortLabel = new TitleLabel("Port", subColor);
    qbitPortLabel->setContentsMargins(sizeScale(tabWidth), 0, 0, 0);
    layout->addWidget(qbitPortLabel, row, 0);
    qbitPortSpin = new DarkSpinBox();
    qbitPortSpin->setRange(1, 65535);
    qbitPortSpin->setValue(8080);
    qbitPortSpin->setFixedWidth(inputWidth);
    layout->addWidget(qbitPortSpin, row++, 1, 1, 2);

    // qBittorrent Username
    TitleLabel* qbitUserLabel = new TitleLabel("Username", subColor);
    qbitUserLabel->setContentsMargins(sizeScale(tabWidth), 0, 0, 0);
    layout->addWidget(qbitUserLabel, row, 0);
    qbitUsernameEdit = new DarkLineEdit();
    qbitUsernameEdit->setFixedWidth(inputWidth);
    layout->addWidget(qbitUsernameEdit, row++, 1, 1, 2);

    // qBittorrent Password
    TitleLabel* qbitPassLabel = new TitleLabel("Password", subColor);
    qbitPassLabel->setContentsMargins(sizeScale(tabWidth), 0, 0, 0);
    layout->addWidget(qbitPassLabel, row, 0);
    qbitPasswordEdit = new DarkLineEdit();
    qbitPasswordEdit->setEchoMode(QLineEdit::Password);
    qbitPasswordEdit->setFixedWidth(inputWidth);
    layout->addWidget(qbitPasswordEdit, row++, 1, 1, 2);

    // 4. Auto start program
    autoStartCheck = new ToggleSwitch();
    TitleLabel* autoStartLabel = new TitleLabel("Run at startup", titleColor);
    autoStartLabel->setContentsMargins(sizeScale(tabWidth / 2), 0, 0, 0);
    autoStartLabel->setFont(sectionFont);
    layout->addWidget(autoStartLabel, row, 0, Qt::AlignVCenter);
    layout->addWidget(autoStartCheck, row++, 2, Qt::AlignRight);

    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
}

void SettingWidget::backToMain() {
    saveToGlobalConfig();
    if (closeClicked) {
        closeClicked();
    }
}

void SettingWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void SettingWidget::closeEvent(QCloseEvent* event) {
    saveToGlobalConfig();
    QWidget::closeEvent(event);
}

void SettingWidget::loadFromGlobalConfig() {
    const SettingConfig& config = GlobalConfig::config;

    httpProxyCheck->setChecked(config.enableHttpProxy);
    proxyAddressEdit->setText(config.proxyAddress.c_str());
    proxyPortSpin->setValue(config.proxyPort);

    qbitIPEdit->setText(config.qbittorrentIP.c_str());
    qbitPortSpin->setValue(config.qbittorrentPort);
    qbitUsernameEdit->setText(config.qbittorrentUsername.c_str());
    qbitPasswordEdit->setText(config.qbittorrentPassword.c_str());

    autoStartCheck->setChecked(config.autoStart);
}

void SettingWidget::saveToGlobalConfig() {
    SettingConfig& config = GlobalConfig::config;

    config.enableHttpProxy = httpProxyCheck->isChecked();
    config.proxyAddress = proxyAddressEdit->text().toStdString();
    config.proxyPort = proxyPortSpin->value();

    config.qbittorrentIP = qbitIPEdit->text().toStdString();
    config.qbittorrentPort = qbitPortSpin->value();
    config.qbittorrentUsername = qbitUsernameEdit->text().toStdString();
    config.qbittorrentPassword = qbitPasswordEdit->text().toStdString();

    config.autoStart = autoStartCheck->isChecked();
    config.saveToFile();
    setAutoRun(config.autoStart, L"AnimeRss");
}
