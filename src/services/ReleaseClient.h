#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class QFile;
class QNetworkReply;
class QUrl;

class ReleaseClient : public QObject
{
    Q_OBJECT

public:
    struct ReleaseInfo
    {
        QString tagName;
        QString publishedAtIsoUtc;
        QString zipballUrl;
        QString lceWindowsZipUrl;
    };

    explicit ReleaseClient(QObject *parent = nullptr);

    void requestNightlyRelease();
    void downloadToFile(const QUrl &url, const QString &outputPath);

signals:
    void releaseReady(const ReleaseClient::ReleaseInfo &info);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &outputPath);
    void errorOccurred(const QString &message);

private slots:
    void onReleaseReplyFinished();
    void onDownloadReadyRead();
    void onDownloadReplyFinished();

private:
    QNetworkAccessManager network;
    QNetworkReply *releaseReply = nullptr;
    QNetworkReply *downloadReply = nullptr;
    QFile *downloadFile = nullptr;
    QString downloadTargetPath;
};
