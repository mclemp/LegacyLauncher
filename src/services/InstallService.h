#pragma once

#include <QString>
#include <QStringList>

namespace InstallService
{
QString defaultInstallRoot();
QString findGameExecutable(const QString &rootDir);
QString detectSaveDirectory(const QString &installDir);
QStringList detectionRoots(const QString &currentInstallDir, const QString &storedInstallDir, const QString &activeProfile);
bool copyRecursively(const QString &sourcePath, const QString &targetPath, QString &errorText);
bool extractZipToDirectory(const QString &zipPath, const QString &destinationDir, QString &errorText);
}
