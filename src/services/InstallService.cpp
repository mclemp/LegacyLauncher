#include "InstallService.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace InstallService
{
QString defaultInstallRoot()
{
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docs.isEmpty()) {
        return QDir::homePath() + "/MinecraftLegacyLauncher/installs";
    }

    return docs + "/MinecraftLegacyLauncher/installs";
}

QString findGameExecutable(const QString &rootDir)
{
    if (rootDir.trimmed().isEmpty()) {
        return {};
    }

    const QString direct = QDir(rootDir).filePath("Minecraft.Client.exe");
    if (QFileInfo::exists(direct)) {
        return direct;
    }

    QDirIterator it(rootDir, QStringList() << "Minecraft.Client.exe", QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext()) {
        return it.next();
    }

    return {};
}

QString detectSaveDirectory(const QString &installDir)
{
    if (installDir.trimmed().isEmpty()) {
        return {};
    }

    const QStringList candidates = {
        QDir(installDir).filePath("saves"),
        QDir(installDir).filePath("SaveData"),
        QDir(installDir).filePath("worlds")
    };

    for (const QString &path : candidates) {
        if (QFileInfo(path).exists() && QFileInfo(path).isDir()) {
            return path;
        }
    }

    return QDir(installDir).filePath("saves");
}

QStringList detectionRoots(const QString &currentInstallDir, const QString &storedInstallDir, const QString &activeProfile)
{
    QStringList roots;
    roots << currentInstallDir;
    roots << storedInstallDir;
    roots << defaultInstallRoot() + "/" + activeProfile;
    roots << defaultInstallRoot() + "/MinecraftConsoles";
    roots << QDir::homePath() + "/Downloads/MinecraftConsoles";
    roots << "C:/MinecraftConsoles";

    roots.removeAll(QString());
    roots.removeDuplicates();
    return roots;
}

bool copyRecursively(const QString &sourcePath, const QString &targetPath, QString &errorText)
{
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) {
        errorText = QString("Source does not exist: %1").arg(sourcePath);
        return false;
    }

    if (sourceInfo.isFile()) {
        QDir().mkpath(QFileInfo(targetPath).absolutePath());
        QFile::remove(targetPath);
        if (!QFile::copy(sourcePath, targetPath)) {
            errorText = QString("Failed to copy file %1 to %2").arg(sourcePath, targetPath);
            return false;
        }
        return true;
    }

    if (!QFileInfo::exists(targetPath) && !QDir().mkpath(targetPath)) {
        errorText = QString("Failed to create directory: %1").arg(targetPath);
        return false;
    }

    QDir sourceDir(sourcePath);
    const QFileInfoList entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    for (const QFileInfo &entry : entries) {
        const QString src = entry.absoluteFilePath();
        const QString dst = QDir(targetPath).filePath(entry.fileName());
        if (!copyRecursively(src, dst, errorText)) {
            return false;
        }
    }

    return true;
}

bool extractZipToDirectory(const QString &zipPath, const QString &destinationDir, QString &errorText)
{
#ifdef Q_OS_WIN
    QString source = QDir::toNativeSeparators(zipPath);
    QString destination = QDir::toNativeSeparators(destinationDir);
    source.replace("'", "''");
    destination.replace("'", "''");

    const QString script = QString("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force").arg(source, destination);
    const int code = QProcess::execute("powershell", {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", script});
    if (code == 0) {
        return true;
    }

    errorText = "Archive extraction failed";
    return false;
#else
    Q_UNUSED(zipPath);
    Q_UNUSED(destinationDir);
    errorText = "Archive extraction is currently only implemented on Windows";
    return false;
#endif
}
}
