#include "ReleaseClient.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

ReleaseClient::ReleaseClient(QObject *parent)
    : QObject(parent)
{
}

void ReleaseClient::requestNightlyRelease()
{
    if (releaseReply) {
        releaseReply->abort();
        releaseReply->deleteLater();
        releaseReply = nullptr;
    }

    QNetworkRequest request{QUrl("https://api.github.com/repos/MLE-MP/MinecraftConsoles/releases/tags/nightly")};
    request.setHeader(QNetworkRequest::UserAgentHeader, "MinecraftLegacyLauncher");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    releaseReply = network.get(request);
    connect(releaseReply, &QNetworkReply::finished, this, &ReleaseClient::onReleaseReplyFinished);
}

void ReleaseClient::downloadToFile(const QUrl &url, const QString &outputPath)
{
    if (downloadReply) {
        downloadReply->abort();
        downloadReply->deleteLater();
        downloadReply = nullptr;
    }

    if (downloadFile) {
        downloadFile->close();
        downloadFile->deleteLater();
        downloadFile = nullptr;
    }

    downloadTargetPath = outputPath;
    downloadFile = new QFile(downloadTargetPath, this);
    if (!downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred(QString("Could not create download target file: %1").arg(downloadTargetPath));
        downloadFile->deleteLater();
        downloadFile = nullptr;
        return;
    }

    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "MinecraftLegacyLauncher");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    downloadReply = network.get(request);
    connect(downloadReply, &QNetworkReply::readyRead, this, &ReleaseClient::onDownloadReadyRead);
    connect(downloadReply, &QNetworkReply::downloadProgress, this, &ReleaseClient::downloadProgress);
    connect(downloadReply, &QNetworkReply::finished, this, &ReleaseClient::onDownloadReplyFinished);
}

void ReleaseClient::onReleaseReplyFinished()
{
    QNetworkReply *reply = releaseReply;
    releaseReply = nullptr;

    if (!reply) {
        return;
    }

    const QByteArray payload = reply->readAll();
    const bool failed = reply->error() != QNetworkReply::NoError;
    const QString networkError = reply->errorString();
    reply->deleteLater();

    if (failed) {
        emit errorOccurred(QString("Release check failed: %1").arg(networkError));
        return;
    }

    const QJsonDocument json = QJsonDocument::fromJson(payload);
    if (!json.isObject()) {
        emit errorOccurred("Release metadata was invalid JSON");
        return;
    }

    const QJsonObject root = json.object();
    ReleaseInfo info;
    info.tagName = root.value("tag_name").toString("nightly");
    info.publishedAtIsoUtc = root.value("published_at").toString();
    info.zipballUrl = root.value("zipball_url").toString();

    const QJsonArray assets = root.value("assets").toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value("name").toString();
        if (name.compare("LCEWindows64.zip", Qt::CaseInsensitive) == 0) {
            info.lceWindowsZipUrl = asset.value("browser_download_url").toString();
            break;
        }
    }

    emit releaseReady(info);
}

void ReleaseClient::onDownloadReadyRead()
{
    if (!downloadReply || !downloadFile) {
        return;
    }

    downloadFile->write(downloadReply->readAll());
}

void ReleaseClient::onDownloadReplyFinished()
{
    QNetworkReply *reply = downloadReply;
    downloadReply = nullptr;

    if (!reply) {
        return;
    }

    if (downloadFile) {
        downloadFile->write(reply->readAll());
        downloadFile->flush();
        downloadFile->close();
        downloadFile->deleteLater();
        downloadFile = nullptr;
    }

    const bool failed = reply->error() != QNetworkReply::NoError;
    const QString networkError = reply->errorString();
    reply->deleteLater();

    if (failed) {
        emit errorOccurred(QString("Download failed: %1").arg(networkError));
        return;
    }

    emit downloadFinished(downloadTargetPath);
}
