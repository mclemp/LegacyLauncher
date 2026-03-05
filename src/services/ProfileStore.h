#pragma once

#include <QSettings>
#include <QString>

struct ProfileData
{
    QString installDir;
    QString executablePath;
    QString arguments;
    QString username;
    qint64 playtimeSeconds = 0;
    QString lastPlayedIsoUtc;
    QString installedReleaseTag;
    QString installedReleasePublishedAtIsoUtc;
};

class ProfileStore
{
public:
    explicit ProfileStore(const QString &organization = "MLE-MP", const QString &application = "MinecraftLegacyLauncher");

    QString activeProfile(const QString &fallback) const;
    void setActiveProfile(const QString &profile);

    QString themeKey(const QString &fallback = "dark") const;
    void setThemeKey(const QString &theme);

    ProfileData loadProfile(const QString &profile) const;
    void saveProfile(const QString &profile, const ProfileData &data);

private:
    QString key(const QString &profile, const QString &field) const;

    mutable QSettings settings;
};
