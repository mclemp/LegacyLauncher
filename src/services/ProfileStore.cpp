#include "ProfileStore.h"

ProfileStore::ProfileStore(const QString &organization, const QString &application)
    : settings(organization, application)
{
}

QString ProfileStore::activeProfile(const QString &fallback) const
{
    return settings.value("ui/activeProfile", fallback).toString();
}

void ProfileStore::setActiveProfile(const QString &profile)
{
    settings.setValue("ui/activeProfile", profile);
}

QString ProfileStore::themeKey(const QString &fallback) const
{
    return settings.value("ui/theme", fallback).toString();
}

void ProfileStore::setThemeKey(const QString &theme)
{
    settings.setValue("ui/theme", theme);
}

ProfileData ProfileStore::loadProfile(const QString &profile) const
{
    ProfileData data;
    data.installDir = settings.value(key(profile, "installDir")).toString();
    data.executablePath = settings.value(key(profile, "executable")).toString();
    data.arguments = settings.value(key(profile, "arguments")).toString();
    data.username = settings.value(key(profile, "username")).toString();
    data.playtimeSeconds = settings.value(key(profile, "playtimeSeconds"), 0).toLongLong();
    data.lastPlayedIsoUtc = settings.value(key(profile, "lastPlayed")).toString();
    data.installedReleaseTag = settings.value(key(profile, "installedReleaseTag")).toString();
    data.installedReleasePublishedAtIsoUtc = settings.value(key(profile, "installedReleasePublishedAt")).toString();
    return data;
}

void ProfileStore::saveProfile(const QString &profile, const ProfileData &data)
{
    settings.setValue(key(profile, "installDir"), data.installDir);
    settings.setValue(key(profile, "executable"), data.executablePath);
    settings.setValue(key(profile, "arguments"), data.arguments);
    settings.setValue(key(profile, "username"), data.username);
    settings.setValue(key(profile, "playtimeSeconds"), data.playtimeSeconds);
    settings.setValue(key(profile, "lastPlayed"), data.lastPlayedIsoUtc);
    settings.setValue(key(profile, "installedReleaseTag"), data.installedReleaseTag);
    settings.setValue(key(profile, "installedReleasePublishedAt"), data.installedReleasePublishedAtIsoUtc);
}

QString ProfileStore::key(const QString &profile, const QString &field) const
{
    return QString("profiles/%1/%2").arg(profile, field);
}
