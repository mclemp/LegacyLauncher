#pragma once

#include <QMainWindow>
#include <QProcess>

#include "services/ProfileStore.h"
#include "services/ReleaseClient.h"

class QComboBox;
class QLineEdit;
class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onProfileChanged(const QString &profile);
    void browseInstallDirectory();
    void detectInstallations();
    void downloadAndInstall();
    void checkForUpdates();

    void launchGame();
    void onGameFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onGameError(QProcess::ProcessError error);

    void backupSaves();
    void restoreSaves();

    void saveSettings();
    void applySelectedTheme();

    void onReleaseReady(const ReleaseClient::ReleaseInfo &info);
    void onReleaseError(const QString &message);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(const QString &outputPath);

private:
    void setupUi();
    void loadSettings();
    void loadProfile(const QString &profile);
    void saveCurrentProfile();
    void runStartupGamePrompt();

    void startDownloadFromRelease(const ReleaseClient::ReleaseInfo &info);
    bool installDownloadedArchive(const QString &archivePath);

    void setGameLocation(const QString &executablePath);
    void updateStats();
    void appendLog(const QString &line);
    void applyTheme(const QString &themeKey);
    void updateStatus(const QString &message, bool isError = false);

    ProfileStore profileStore;
    ReleaseClient releaseClient;

    bool loadingProfile = false;
    bool installAfterReleaseCheck = false;
    bool downloadIsArchive = false;

    QString activeProfile;
    QString currentInstallDir;
    QString currentExecutablePath;

    QString pendingInstallDir;
    QString pendingDownloadedPath;
    QString pendingReleaseTag;
    QString pendingReleasePublishedAt;

    QProcess *gameProcess = nullptr;
    qint64 sessionStartMs = 0;

    QComboBox *profileCombo = nullptr;
    QComboBox *themeCombo = nullptr;
    QLineEdit *argsEdit = nullptr;
    QLineEdit *usernameEdit = nullptr;

    QLabel *statusLabel = nullptr;
    QLabel *updateLabel = nullptr;
    QLabel *statsLabel = nullptr;
    QLabel *gameLocationLabel = nullptr;

    QProgressBar *downloadProgress = nullptr;
    QTextEdit *logView = nullptr;

    QPushButton *launchButton = nullptr;
    QPushButton *downloadButton = nullptr;
};
