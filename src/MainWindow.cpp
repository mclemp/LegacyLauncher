#include "MainWindow.h"

#include "services/InstallService.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Minecraft Legacy Launcher");
    setWindowIcon(QIcon(":/icons/app.svg"));
    setMinimumSize(860, 650);

    setupUi();
    loadSettings();

    connect(&releaseClient, &ReleaseClient::releaseReady, this, &MainWindow::onReleaseReady);
    connect(&releaseClient, &ReleaseClient::errorOccurred, this, &MainWindow::onReleaseError);
    connect(&releaseClient, &ReleaseClient::downloadProgress, this, &MainWindow::onDownloadProgress);
    connect(&releaseClient, &ReleaseClient::downloadFinished, this, &MainWindow::onDownloadFinished);

    QTimer::singleShot(300, this, [this]() {
        checkForUpdates();
    });
    QTimer::singleShot(500, this, [this]() {
        runStartupGamePrompt();
    });

    updateStatus("Ready");
    appendLog("Launcher initialized");
}

void MainWindow::setupUi()
{
    auto *root = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout;

    auto *logo = new QLabel;
    logo->setPixmap(QIcon(":/icons/app.svg").pixmap(36, 36));

    auto *titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(1);

    auto *title = new QLabel("Minecraft Legacy Launcher");
    title->setObjectName("Title");

    auto *subtitle = new QLabel("Download, install, update, and launch from one place.");
    subtitle->setObjectName("Subtitle");

    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);

    themeCombo = new QComboBox;
    themeCombo->addItem("Dark", "dark");
    themeCombo->addItem("Light", "light");

    headerLayout->addWidget(logo);
    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(new QLabel("Theme"));
    headerLayout->addWidget(themeCombo);

    auto *card = new QFrame;
    card->setObjectName("Card");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(10);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(9);

    profileCombo = new QComboBox;
    profileCombo->addItems({"Default", "Xbox 360", "PlayStation 3", "PlayStation Vita", "Wii U"});

    argsEdit = new QLineEdit;
    argsEdit->setPlaceholderText("Launch arguments, e.g. -name Steve -ip 127.0.0.1");

    usernameEdit = new QLineEdit;
    usernameEdit->setPlaceholderText("Optional in-game username");

    int row = 0;
    grid->addWidget(new QLabel("Profile"), row, 0);
    grid->addWidget(profileCombo, row, 1, 1, 3);
    row++;

    gameLocationLabel = new QLabel("Game location: not found");
    gameLocationLabel->setObjectName("Subtle");
    grid->addWidget(gameLocationLabel, row, 0, 1, 4);
    row++;

    grid->addWidget(new QLabel("Username"), row, 0);
    grid->addWidget(usernameEdit, row, 1, 1, 3);
    row++;

    grid->addWidget(new QLabel("Arguments"), row, 0);
    grid->addWidget(argsEdit, row, 1, 1, 3);

    auto *buttonRow = new QHBoxLayout;

    downloadButton = new QPushButton("Download / Install");
    downloadButton->setObjectName("PrimaryAction");

    auto *updatesButton = new QPushButton("Check Updates");
    auto *locateButton = new QPushButton("Locate Game");
    auto *backupButton = new QPushButton("Backup Saves");
    auto *restoreButton = new QPushButton("Restore Saves");

    launchButton = new QPushButton("Launch");
    launchButton->setObjectName("LaunchButton");
    launchButton->setIcon(QIcon(":/icons/play.svg"));
    launchButton->setMinimumHeight(38);

    buttonRow->addWidget(downloadButton);
    buttonRow->addWidget(updatesButton);
    buttonRow->addWidget(locateButton);
    buttonRow->addWidget(backupButton);
    buttonRow->addWidget(restoreButton);
    buttonRow->addStretch();
    buttonRow->addWidget(launchButton);

    downloadProgress = new QProgressBar;
    downloadProgress->setMinimum(0);
    downloadProgress->setMaximum(100);
    downloadProgress->setValue(0);

    updateLabel = new QLabel("Update status: unknown");
    updateLabel->setObjectName("Subtle");

    statsLabel = new QLabel("Playtime: 0m");
    statsLabel->setObjectName("Subtle");

    statusLabel = new QLabel;
    statusLabel->setObjectName("Status");

    cardLayout->addLayout(grid);
    cardLayout->addLayout(buttonRow);
    cardLayout->addWidget(downloadProgress);
    cardLayout->addWidget(updateLabel);
    cardLayout->addWidget(statsLabel);
    cardLayout->addWidget(statusLabel);

    auto *logsCard = new QFrame;
    logsCard->setObjectName("Card");

    auto *logsLayout = new QVBoxLayout(logsCard);
    logsLayout->setContentsMargins(12, 12, 12, 12);
    logsLayout->setSpacing(8);

    auto *logsTitle = new QLabel("Activity");
    logsTitle->setObjectName("FieldLabel");

    logView = new QTextEdit;
    logView->setReadOnly(true);
    logView->setMinimumHeight(180);

    logsLayout->addWidget(logsTitle);
    logsLayout->addWidget(logView);

    rootLayout->addLayout(headerLayout);
    rootLayout->addWidget(card);
    rootLayout->addWidget(logsCard, 1);

    setCentralWidget(root);

    connect(themeCombo, &QComboBox::currentTextChanged, this, &MainWindow::applySelectedTheme);
    connect(profileCombo, &QComboBox::currentTextChanged, this, &MainWindow::onProfileChanged);
    connect(locateButton, &QPushButton::clicked, this, &MainWindow::browseInstallDirectory);
    connect(downloadButton, &QPushButton::clicked, this, &MainWindow::downloadAndInstall);
    connect(updatesButton, &QPushButton::clicked, this, &MainWindow::checkForUpdates);
    connect(backupButton, &QPushButton::clicked, this, &MainWindow::backupSaves);
    connect(restoreButton, &QPushButton::clicked, this, &MainWindow::restoreSaves);
    connect(launchButton, &QPushButton::clicked, this, &MainWindow::launchGame);

    connect(argsEdit, &QLineEdit::textChanged, this, &MainWindow::saveSettings);
    connect(usernameEdit, &QLineEdit::textChanged, this, &MainWindow::saveSettings);
}

void MainWindow::loadSettings()
{
    const QString theme = profileStore.themeKey("dark");
    const int themeIndex = themeCombo->findData(theme);
    if (themeIndex >= 0) {
        themeCombo->setCurrentIndex(themeIndex);
    }
    applyTheme(themeCombo->currentData().toString());

    const QString profile = profileStore.activeProfile(profileCombo->currentText());
    const int profileIndex = profileCombo->findText(profile);
    if (profileIndex >= 0) {
        profileCombo->setCurrentIndex(profileIndex);
    }

    activeProfile = profileCombo->currentText();
    loadProfile(activeProfile);
}

void MainWindow::loadProfile(const QString &profile)
{
    loadingProfile = true;
    activeProfile = profile;

    ProfileData data = profileStore.loadProfile(profile);
    if (data.installDir.isEmpty()) {
        data.installDir = InstallService::defaultInstallRoot() + "/" + profile;
    }
    if (data.executablePath.isEmpty()) {
        data.executablePath = InstallService::findGameExecutable(data.installDir);
    }

    currentInstallDir = data.installDir;
    currentExecutablePath = data.executablePath;

    argsEdit->setText(data.arguments);
    usernameEdit->setText(data.username);

    setGameLocation(currentExecutablePath);
    updateStats();

    loadingProfile = false;
}

void MainWindow::saveCurrentProfile()
{
    if (loadingProfile || activeProfile.isEmpty()) {
        return;
    }

    ProfileData data = profileStore.loadProfile(activeProfile);
    data.installDir = currentInstallDir;
    data.executablePath = currentExecutablePath;
    data.arguments = argsEdit->text().trimmed();
    data.username = usernameEdit->text().trimmed();

    profileStore.saveProfile(activeProfile, data);
    profileStore.setActiveProfile(activeProfile);
}

void MainWindow::runStartupGamePrompt()
{
    detectInstallations();
    if (!currentExecutablePath.isEmpty() && QFileInfo::exists(currentExecutablePath)) {
        return;
    }

    QMessageBox prompt(this);
    prompt.setWindowTitle("Game files required");
    prompt.setIcon(QMessageBox::Information);
    prompt.setText("MinecraftConsoles files were not found for this profile.");
    prompt.setInformativeText("Choose Browse Location to use an existing install, or Download to fetch and unzip the repository automatically.");

    QPushButton *browseButton = prompt.addButton("Browse Location", QMessageBox::AcceptRole);
    QPushButton *downloadPromptButton = prompt.addButton("Download", QMessageBox::ActionRole);
    prompt.addButton(QMessageBox::Cancel);

    prompt.exec();

    if (prompt.clickedButton() == browseButton) {
        browseInstallDirectory();
    } else if (prompt.clickedButton() == downloadPromptButton) {
        downloadAndInstall();
    }
}

void MainWindow::onProfileChanged(const QString &profile)
{
    if (profile == activeProfile) {
        return;
    }

    saveCurrentProfile();
    loadProfile(profile);
    appendLog(QString("Switched profile to %1").arg(profile));
}

void MainWindow::browseInstallDirectory()
{
    const QString base = currentInstallDir.isEmpty() ? InstallService::defaultInstallRoot() : currentInstallDir;
    const QString selected = QFileDialog::getExistingDirectory(this, "Select install folder", base);
    if (selected.isEmpty()) {
        return;
    }

    currentInstallDir = selected;
    const QString detected = InstallService::findGameExecutable(selected);

    if (!detected.isEmpty()) {
        currentExecutablePath = detected;
        setGameLocation(currentExecutablePath);
        updateStatus("Game files found");
        appendLog(QString("Game location selected: %1").arg(detected));
        saveSettings();
        return;
    }

    currentExecutablePath.clear();
    setGameLocation({});
    updateStatus("Minecraft.Client.exe was not found in that location", true);
    saveSettings();
}

void MainWindow::detectInstallations()
{
    const ProfileData stored = profileStore.loadProfile(activeProfile);
    const QStringList roots = InstallService::detectionRoots(currentInstallDir, stored.installDir, activeProfile);

    for (const QString &root : roots) {
        const QString found = InstallService::findGameExecutable(root);
        if (!found.isEmpty()) {
            currentInstallDir = root;
            currentExecutablePath = found;
            setGameLocation(currentExecutablePath);
            updateStatus("Detected existing installation");
            appendLog(QString("Auto-detected executable: %1").arg(found));
            saveSettings();
            return;
        }
    }

    currentExecutablePath.clear();
    setGameLocation({});
    updateStatus("No installation detected in common locations", true);
    appendLog("Auto-detect did not find Minecraft.Client.exe");
}

void MainWindow::downloadAndInstall()
{
    pendingInstallDir = currentInstallDir;
    if (pendingInstallDir.isEmpty()) {
        pendingInstallDir = InstallService::defaultInstallRoot() + "/" + activeProfile;
        currentInstallDir = pendingInstallDir;
    }

    QDir().mkpath(pendingInstallDir);
    installAfterReleaseCheck = true;
    updateStatus("Checking release metadata for installation");
    releaseClient.requestNightlyRelease();
}

void MainWindow::checkForUpdates()
{
    installAfterReleaseCheck = false;
    updateStatus("Checking release metadata");
    releaseClient.requestNightlyRelease();
}

void MainWindow::onReleaseReady(const ReleaseClient::ReleaseInfo &info)
{
    pendingReleaseTag = info.tagName;
    pendingReleasePublishedAt = info.publishedAtIsoUtc;

    if (!installAfterReleaseCheck) {
        const ProfileData data = profileStore.loadProfile(activeProfile);
        const QDateTime remote = QDateTime::fromString(info.publishedAtIsoUtc, Qt::ISODate);
        const QDateTime local = QDateTime::fromString(data.installedReleasePublishedAtIsoUtc, Qt::ISODate);

        if (!remote.isValid()) {
            updateLabel->setText("Update status: unable to parse release timestamp");
        } else if (!local.isValid() || remote > local) {
            updateLabel->setText(QString("Update available: %1 (%2)").arg(info.tagName, remote.toLocalTime().toString()));
        } else {
            updateLabel->setText(QString("Up to date: %1").arg(info.tagName));
        }

        updateStatus("Update check complete");
        appendLog("Update metadata fetched");
        return;
    }

    startDownloadFromRelease(info);
}

void MainWindow::startDownloadFromRelease(const ReleaseClient::ReleaseInfo &info)
{
    QString chosenUrl = info.zipballUrl;
    QString chosenName = QString("MinecraftConsoles-%1.zip").arg(info.tagName.isEmpty() ? "nightly" : info.tagName);

    if (chosenUrl.isEmpty()) {
        chosenUrl = info.lceWindowsZipUrl;
        chosenName = "LCEWindows64.zip";
    }

    if (chosenUrl.isEmpty()) {
        updateStatus("No downloadable repo/archive asset found in nightly release", true);
        appendLog("Installer failed: release metadata had no zipball_url or LCEWindows64.zip");
        return;
    }

    downloadIsArchive = chosenName.endsWith(".zip", Qt::CaseInsensitive);
    if (downloadIsArchive) {
        const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/MinecraftLegacyLauncher";
        QDir().mkpath(tempRoot);
        pendingDownloadedPath = QDir(tempRoot).filePath(chosenName);
    } else {
        pendingDownloadedPath = QDir(pendingInstallDir).filePath(chosenName);
    }

    downloadProgress->setMaximum(100);
    downloadProgress->setValue(0);

    updateStatus(QString("Downloading %1").arg(chosenName));
    appendLog(QString("Downloading asset from %1").arg(chosenUrl));

    releaseClient.downloadToFile(QUrl(chosenUrl), pendingDownloadedPath);
}

void MainWindow::onReleaseError(const QString &message)
{
    updateStatus(message, true);
    appendLog(message);
}

void MainWindow::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) {
        downloadProgress->setMaximum(0);
        return;
    }

    downloadProgress->setMaximum(100);
    const int pct = static_cast<int>((100.0 * bytesReceived) / bytesTotal);
    downloadProgress->setValue(qBound(0, pct, 100));
}

void MainWindow::onDownloadFinished(const QString &outputPath)
{
    pendingDownloadedPath = outputPath;
    if (!installDownloadedArchive(outputPath)) {
        return;
    }

    downloadProgress->setValue(100);
}

bool MainWindow::installDownloadedArchive(const QString &archivePath)
{
    if (pendingInstallDir.isEmpty()) {
        updateStatus("Install folder is not set", true);
        return false;
    }

    QDir().mkpath(pendingInstallDir);

    if (downloadIsArchive) {
        QString errorText;
        if (!InstallService::extractZipToDirectory(archivePath, pendingInstallDir, errorText)) {
            updateStatus(errorText, true);
            appendLog(errorText);
            return false;
        }
    }

    QString executable = InstallService::findGameExecutable(pendingInstallDir);
    if (executable.isEmpty() && !downloadIsArchive && QFileInfo::exists(archivePath)) {
        executable = archivePath;
    }

    if (executable.isEmpty()) {
        updateStatus("Install completed but executable was not found", true);
        appendLog("Install finished but Minecraft.Client.exe was not found");
        return false;
    }

    currentInstallDir = pendingInstallDir;
    currentExecutablePath = executable;
    setGameLocation(currentExecutablePath);

    ProfileData data = profileStore.loadProfile(activeProfile);
    data.installDir = currentInstallDir;
    data.executablePath = currentExecutablePath;
    data.installedReleaseTag = pendingReleaseTag;
    data.installedReleasePublishedAtIsoUtc = pendingReleasePublishedAt;
    profileStore.saveProfile(activeProfile, data);

    updateStatus("Installation complete");
    appendLog(QString("Installed %1 into %2").arg(pendingReleaseTag, pendingInstallDir));
    return true;
}

void MainWindow::launchGame()
{
    if (gameProcess && gameProcess->state() != QProcess::NotRunning) {
        updateStatus("Game is already running", true);
        return;
    }

    QString executablePath = currentExecutablePath;
    if (executablePath.isEmpty()) {
        detectInstallations();
        executablePath = currentExecutablePath;
    }

    if (executablePath.isEmpty() || !QFileInfo::exists(executablePath)) {
        runStartupGamePrompt();
        executablePath = currentExecutablePath;
    }

    if (executablePath.isEmpty() || !QFileInfo::exists(executablePath)) {
        updateStatus("Launch cancelled: game files are still missing", true);
        return;
    }

    QStringList arguments = QProcess::splitCommand(argsEdit->text().trimmed());
    const QString username = usernameEdit->text().trimmed();

    bool hasName = false;
    for (const QString &arg : arguments) {
        if (arg == "-name") {
            hasName = true;
            break;
        }
    }

    if (!username.isEmpty() && !hasName) {
        arguments << "-name" << username;
    }

    if (gameProcess) {
        gameProcess->deleteLater();
    }

    gameProcess = new QProcess(this);
    gameProcess->setProgram(executablePath);
    gameProcess->setArguments(arguments);
    gameProcess->setWorkingDirectory(QFileInfo(executablePath).absolutePath());

    connect(gameProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &MainWindow::onGameFinished);
    connect(gameProcess, &QProcess::errorOccurred, this, &MainWindow::onGameError);

    gameProcess->start();
    if (!gameProcess->waitForStarted(8000)) {
        updateStatus("Launch failed. Verify game dependencies", true);
        appendLog("QProcess failed to start Minecraft.Client.exe");
        gameProcess->deleteLater();
        gameProcess = nullptr;
        return;
    }

    sessionStartMs = QDateTime::currentMSecsSinceEpoch();

    ProfileData data = profileStore.loadProfile(activeProfile);
    data.installDir = currentInstallDir;
    data.executablePath = executablePath;
    data.lastPlayedIsoUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    profileStore.saveProfile(activeProfile, data);

    currentExecutablePath = executablePath;
    launchButton->setEnabled(false);

    updateStatus(QString("Launched %1").arg(activeProfile));
    appendLog(QString("Launched executable: %1").arg(executablePath));
    saveCurrentProfile();
    updateStats();
}

void MainWindow::onGameFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    launchButton->setEnabled(true);

    const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - sessionStartMs);
    const qint64 elapsedSec = elapsedMs / 1000;

    ProfileData data = profileStore.loadProfile(activeProfile);
    data.playtimeSeconds += elapsedSec;
    data.lastPlayedIsoUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    profileStore.saveProfile(activeProfile, data);

    updateStats();
    updateStatus(QString("Game exited with code %1").arg(exitCode), exitCode != 0 || exitStatus != QProcess::NormalExit);
    appendLog(QString("Game exited. code=%1 status=%2 session=%3s")
              .arg(exitCode)
              .arg(exitStatus == QProcess::NormalExit ? "normal" : "crash")
              .arg(elapsedSec));

    if (gameProcess) {
        gameProcess->deleteLater();
        gameProcess = nullptr;
    }
}

void MainWindow::onGameError(QProcess::ProcessError error)
{
    launchButton->setEnabled(true);
    updateStatus(QString("Game process error: %1").arg(static_cast<int>(error)), true);
    appendLog(QString("QProcess error occurred: %1").arg(static_cast<int>(error)));
}

void MainWindow::backupSaves()
{
    const QString saveDir = InstallService::detectSaveDirectory(currentInstallDir);
    if (!QFileInfo::exists(saveDir)) {
        updateStatus("No save folder found to back up", true);
        return;
    }

    const QString backupRoot = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/MinecraftLegacyLauncherBackups";
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    const QString backupDir = QDir(backupRoot).filePath(activeProfile + "-" + stamp);

    QString errorText;
    if (!InstallService::copyRecursively(saveDir, backupDir, errorText)) {
        updateStatus(errorText, true);
        appendLog(errorText);
        return;
    }

    updateStatus("Save backup complete");
    appendLog(QString("Backed up saves to %1").arg(backupDir));
}

void MainWindow::restoreSaves()
{
    const QString sourceBackup = QFileDialog::getExistingDirectory(this, "Select backup folder", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/MinecraftLegacyLauncherBackups");
    if (sourceBackup.isEmpty()) {
        return;
    }

    const QString targetSaveDir = InstallService::detectSaveDirectory(currentInstallDir);
    if (QMessageBox::question(this, "Confirm restore", "Restore this backup into your active save folder? This overwrites existing files.") != QMessageBox::Yes) {
        return;
    }

    QString errorText;
    if (!InstallService::copyRecursively(sourceBackup, targetSaveDir, errorText)) {
        updateStatus(errorText, true);
        appendLog(errorText);
        return;
    }

    updateStatus("Save restore complete");
    appendLog(QString("Restored saves from %1").arg(sourceBackup));
}

void MainWindow::saveSettings()
{
    if (loadingProfile) {
        return;
    }

    profileStore.setThemeKey(themeCombo->currentData().toString());
    saveCurrentProfile();
}

void MainWindow::applySelectedTheme()
{
    applyTheme(themeCombo->currentData().toString());
    saveSettings();
}

void MainWindow::appendLog(const QString &line)
{
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    const QString full = QString("[%1] %2").arg(stamp, line);

    if (logView) {
        logView->append(full);
    }

    const QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);

    QFile logFile(QDir(logDir).filePath("launcher.log"));
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream << full << Qt::endl;
    }
}

void MainWindow::updateStats()
{
    const ProfileData data = profileStore.loadProfile(activeProfile);
    const qint64 totalMin = data.playtimeSeconds / 60;
    const qint64 hours = totalMin / 60;
    const qint64 mins = totalMin % 60;

    QString lastText = "never";
    if (!data.lastPlayedIsoUtc.isEmpty()) {
        const QDateTime parsed = QDateTime::fromString(data.lastPlayedIsoUtc, Qt::ISODate);
        if (parsed.isValid()) {
            lastText = parsed.toLocalTime().toString();
        }
    }

    statsLabel->setText(QString("Playtime: %1h %2m • Last played: %3").arg(hours).arg(mins).arg(lastText));
}

void MainWindow::updateStatus(const QString &message, bool isError)
{
    statusLabel->setText(message);
    statusLabel->setProperty("error", isError);
    style()->unpolish(statusLabel);
    style()->polish(statusLabel);
}

void MainWindow::setGameLocation(const QString &executablePath)
{
    if (executablePath.trimmed().isEmpty()) {
        gameLocationLabel->setText("Game location: not found");
        return;
    }

    gameLocationLabel->setText(QString("Game location: %1").arg(executablePath));
}

void MainWindow::applyTheme(const QString &themeKey)
{
    static const QStringList preferredFonts = {"Segoe UI", "Inter", "Roboto", "Noto Sans", "Arial"};
    const QFontDatabase db;
    for (const QString &family : preferredFonts) {
        if (db.families().contains(family)) {
            QApplication::setFont(QFont(family, 10));
            break;
        }
    }

    if (themeKey == "light") {
        setStyleSheet(
            "QMainWindow { background: #f4f7fb; color: #172230; }"
            "QFrame#Card { background: #ffffff; border: 1px solid #d7dfeb; border-radius: 12px; }"
            "QLabel#Title { font-size: 21px; font-weight: 700; color: #111827; }"
            "QLabel#Subtitle, QLabel#Subtle { color: #5d6a7c; }"
            "QLabel#FieldLabel { font-size: 12px; font-weight: 600; color: #3e4d63; }"
            "QLineEdit, QComboBox, QTextEdit { background: #ffffff; color: #132032; border: 1px solid #c8d3e3; border-radius: 8px; padding: 5px 8px; }"
            "QPushButton { background: #eef2f8; color: #122136; border: 1px solid #c7d3e4; border-radius: 8px; min-height: 32px; padding: 0 12px; font-weight: 600; }"
            "QPushButton:hover { background: #e2e8f3; }"
            "QPushButton#PrimaryAction, QPushButton#LaunchButton { background: #2d6dff; color: #ffffff; border-color: #2d6dff; }"
            "QPushButton#PrimaryAction:hover, QPushButton#LaunchButton:hover { background: #245fe5; }"
            "QLabel#Status { color: #0f8e53; font-size: 12px; }"
            "QLabel#Status[error='true'] { color: #c8352d; }"
            "QProgressBar { border: 1px solid #c8d3e3; border-radius: 7px; background: #ffffff; text-align: center; }"
            "QProgressBar::chunk { background: #2d6dff; border-radius: 6px; }"
        );
        return;
    }

    setStyleSheet(
        "QMainWindow { background: #0f1115; color: #e7eaf0; }"
        "QFrame#Card { background: #171b22; border: 1px solid #252b36; border-radius: 12px; }"
        "QLabel#Title { font-size: 21px; font-weight: 700; color: #f9fbff; }"
        "QLabel#Subtitle, QLabel#Subtle { color: #96a2b4; }"
        "QLabel#FieldLabel { color: #bac4d4; font-size: 12px; font-weight: 600; }"
        "QComboBox, QLineEdit, QTextEdit { background: #10141a; border: 1px solid #2b3342; border-radius: 8px; color: #e9edf4; padding: 5px 8px; }"
        "QPushButton { min-height: 32px; background: #1c2230; border: 1px solid #2d374b; border-radius: 8px; color: #eef2f7; font-weight: 600; padding: 0 12px; }"
        "QPushButton:hover { background: #242d3f; }"
        "QPushButton:pressed { background: #1a2130; }"
        "QPushButton#PrimaryAction, QPushButton#LaunchButton { background: #3b7cff; border-color: #4e86ff; }"
        "QPushButton#PrimaryAction:hover, QPushButton#LaunchButton:hover { background: #4a86ff; }"
        "QLabel#Status { color: #8dd6a6; font-size: 12px; }"
        "QLabel#Status[error='true'] { color: #ff8f8f; }"
        "QProgressBar { border: 1px solid #2b3342; border-radius: 7px; background: #10141a; text-align: center; color: #d8e2f0; }"
        "QProgressBar::chunk { background: #3b7cff; border-radius: 6px; }"
    );
}
