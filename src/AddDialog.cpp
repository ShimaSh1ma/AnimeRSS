#include "AddDialog.h"
#include <QRegularExpression>

AddDialog::AddDialog(QWidget* parent)
    : QDialog(parent), titleInput(new QLineEdit(this)), rssUrlInput(new QLineEdit(this)), savePathInput(new QLineEdit(this)),
      browseButton(new QPushButton("Browse", this)), addButton(new QPushButton("Add", this)), cancelButton(new QPushButton("Cancel", this)) {

    setWindowModality(Qt::NonModal);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose, true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title
    QHBoxLayout* titleLayout = new QHBoxLayout();
    titleLayout->addWidget(new QLabel("Title:", this));
    titleLayout->addWidget(titleInput);
    mainLayout->addLayout(titleLayout);

    // RSS URL
    QHBoxLayout* rssLayout = new QHBoxLayout();
    rssLayout->addWidget(new QLabel("RSS URL:", this));
    rssLayout->addWidget(rssUrlInput);
    mainLayout->addLayout(rssLayout);

    // Save Path
    QHBoxLayout* savePathLayout = new QHBoxLayout();
    savePathLayout->addWidget(new QLabel("Save Path:", this));
    savePathLayout->addWidget(savePathInput);
    savePathLayout->addWidget(browseButton);
    mainLayout->addLayout(savePathLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Initially disable Add button
    addButton->setEnabled(false);

    connect(browseButton, &QPushButton::clicked, this, &AddDialog::selectSavePath);
    connect(addButton, &QPushButton::clicked, this, [this]() {
        if (onAddClicked) {
            onAddClicked(getRssUrl().toStdString().c_str(), getSavePath().toStdString().c_str(), getTitle().toStdString().c_str());
        }
        accept(); // 关闭对话框
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Validate when any input changes
    connect(titleInput, &QLineEdit::textChanged, this, &AddDialog::validateInput);
    connect(rssUrlInput, &QLineEdit::textChanged, this, &AddDialog::validateInput);
    connect(savePathInput, &QLineEdit::textChanged, this, &AddDialog::validateInput);

    setLayout(mainLayout);
}

QString AddDialog::getTitle() const {
    return titleInput->text();
}

QString AddDialog::getRssUrl() const {
    return rssUrlInput->text();
}

QString AddDialog::getSavePath() const {
    return savePathInput->text();
}

void AddDialog::selectSavePath() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Save Path");
    if (!dir.isEmpty()) {
        savePathInput->setText(dir);
    }
}

void AddDialog::validateInput() {
    const QString urlPattern = R"(^(https?://)[^\s<>"]+$)";
    QRegularExpression regex(urlPattern, QRegularExpression::CaseInsensitiveOption);

    bool valid = !titleInput->text().isEmpty() && !savePathInput->text().isEmpty() && regex.match(rssUrlInput->text()).hasMatch();

    addButton->setEnabled(valid);
}
