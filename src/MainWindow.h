#pragma once

#include <QMainWindow>
#include <QSettings>

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void browseExecutable();
    void launchGame();
    void saveSettings();

private:
    void loadSettings();
    void applyTheme();
    void updateStatus(const QString &message, bool isError = false);

    QSettings settings;
    QComboBox *profileCombo = nullptr;
    QLineEdit *execEdit = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *launchButton = nullptr;
};
