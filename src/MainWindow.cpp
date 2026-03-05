#include "MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      settings("MLE-MP", "MinecraftLegacyLauncher")
{
    setWindowTitle("Minecraft Legacy Launcher");
    setWindowIcon(QIcon(":/icons/app.svg"));
    setMinimumSize(700, 430);

    auto *root = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(28, 24, 28, 24);
    rootLayout->setSpacing(18);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(12);

    auto *logo = new QLabel;
    logo->setPixmap(QIcon(":/icons/app.svg").pixmap(42, 42));

    auto *titleWrap = new QVBoxLayout;
    titleWrap->setSpacing(2);

    auto *title = new QLabel("Minecraft Legacy Launcher");
    title->setObjectName("Title");

    auto *subtitle = new QLabel("Boot your preferred legacy edition profile with one click.");
    subtitle->setObjectName("Subtitle");

    titleWrap->addWidget(title);
    titleWrap->addWidget(subtitle);

    headerLayout->addWidget(logo);
    headerLayout->addLayout(titleWrap);
    headerLayout->addStretch();

    auto *card = new QFrame;
    card->setObjectName("Card");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(14);

    auto *profileLabel = new QLabel("Edition profile");
    profileLabel->setObjectName("FieldLabel");

    profileCombo = new QComboBox;
    profileCombo->addItems({
        "Xbox 360",
        "PlayStation 3",
        "PlayStation Vita",
        "Wii U"
    });

    auto *execLabel = new QLabel("Game executable");
    execLabel->setObjectName("FieldLabel");

    auto *execLayout = new QHBoxLayout;
    execLayout->setSpacing(8);

    execEdit = new QLineEdit;
    execEdit->setPlaceholderText("Select your legacy game executable...");

    auto *browseButton = new QPushButton("Browse");
    browseButton->setIcon(QIcon(":/icons/folder.svg"));

    execLayout->addWidget(execEdit, 1);
    execLayout->addWidget(browseButton);

    launchButton = new QPushButton("Launch");
    launchButton->setObjectName("LaunchButton");
    launchButton->setIcon(QIcon(":/icons/play.svg"));
    launchButton->setMinimumHeight(40);

    statusLabel = new QLabel;
    statusLabel->setObjectName("Status");

    cardLayout->addWidget(profileLabel);
    cardLayout->addWidget(profileCombo);
    cardLayout->addWidget(execLabel);
    cardLayout->addLayout(execLayout);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(launchButton);
    cardLayout->addWidget(statusLabel);

    rootLayout->addLayout(headerLayout);
    rootLayout->addWidget(card);
    rootLayout->addStretch();

    setCentralWidget(root);

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::browseExecutable);
    connect(launchButton, &QPushButton::clicked, this, &MainWindow::launchGame);
    connect(execEdit, &QLineEdit::textChanged, this, &MainWindow::saveSettings);
    connect(profileCombo, &QComboBox::currentTextChanged, this, &MainWindow::saveSettings);

    applyTheme();
    loadSettings();
    updateStatus("Ready.");
}

void MainWindow::browseExecutable()
{
#ifdef Q_OS_WIN
    const QString filter = "Executable (*.exe);;All Files (*.*)";
#else
    const QString filter = "All Files (*)";
#endif

    const QString selected = QFileDialog::getOpenFileName(this, "Select Minecraft legacy executable", execEdit->text(), filter);
    if (selected.isEmpty()) {
        return;
    }

    execEdit->setText(selected);
    updateStatus("Executable selected.");
}

void MainWindow::launchGame()
{
    const QString executablePath = execEdit->text().trimmed();
    if (executablePath.isEmpty()) {
        updateStatus("Pick an executable first.", true);
        return;
    }

    if (!QFileInfo::exists(executablePath)) {
        updateStatus("That file does not exist. Pick a real executable.", true);
        return;
    }

    const bool ok = QProcess::startDetached(executablePath, {}, QFileInfo(executablePath).absolutePath());
    if (!ok) {
        updateStatus("Launch failed. Check permissions and path.", true);
        return;
    }

    updateStatus(QString("Launched %1 profile.").arg(profileCombo->currentText()));
}

void MainWindow::saveSettings()
{
    settings.setValue("profile", profileCombo->currentText());
    settings.setValue("executable", execEdit->text().trimmed());
}

void MainWindow::loadSettings()
{
    const QString profile = settings.value("profile").toString();
    if (!profile.isEmpty()) {
        const int idx = profileCombo->findText(profile);
        if (idx >= 0) {
            profileCombo->setCurrentIndex(idx);
        }
    }

    execEdit->setText(settings.value("executable").toString());
}

void MainWindow::updateStatus(const QString &message, bool isError)
{
    statusLabel->setText(message);
    statusLabel->setProperty("error", isError);
    style()->unpolish(statusLabel);
    style()->polish(statusLabel);
}

void MainWindow::applyTheme()
{
    static const QStringList preferredFonts = {
        "Segoe UI",
        "Inter",
        "Roboto",
        "Noto Sans",
        "Arial"
    };

    const QFontDatabase db;
    for (const QString &family : preferredFonts) {
        if (db.families().contains(family)) {
            QApplication::setFont(QFont(family, 10));
            break;
        }
    }

    setStyleSheet(
        "QMainWindow {"
        "  background: #0f1115;"
        "  color: #e7eaf0;"
        "}"
        "QFrame#Card {"
        "  background: #171b22;"
        "  border: 1px solid #252b36;"
        "  border-radius: 14px;"
        "}"
        "QLabel#Title {"
        "  font-size: 21px;"
        "  font-weight: 700;"
        "  color: #f9fbff;"
        "}"
        "QLabel#Subtitle {"
        "  color: #96a2b4;"
        "  font-size: 12px;"
        "}"
        "QLabel#FieldLabel {"
        "  color: #bac4d4;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QComboBox, QLineEdit {"
        "  min-height: 34px;"
        "  background: #10141a;"
        "  border: 1px solid #2b3342;"
        "  border-radius: 10px;"
        "  color: #e9edf4;"
        "  padding: 0 10px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 28px;"
        "}"
        "QPushButton {"
        "  min-height: 34px;"
        "  background: #1c2230;"
        "  border: 1px solid #2d374b;"
        "  border-radius: 10px;"
        "  color: #eef2f7;"
        "  font-weight: 600;"
        "  padding: 0 14px;"
        "}"
        "QPushButton:hover {"
        "  background: #242d3f;"
        "}"
        "QPushButton:pressed {"
        "  background: #1a2130;"
        "}"
        "QPushButton#LaunchButton {"
        "  background: #3b7cff;"
        "  border-color: #4e86ff;"
        "}"
        "QPushButton#LaunchButton:hover {"
        "  background: #4a86ff;"
        "}"
        "QLabel#Status {"
        "  color: #8dd6a6;"
        "  font-size: 12px;"
        "}"
        "QLabel#Status[error='true'] {"
        "  color: #ff8f8f;"
        "}"
    );
}
