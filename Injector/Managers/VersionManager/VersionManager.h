#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "../DownloadManager/DownloadManager.h"

struct RemoteVersion {
    std::string name;
    std::string host;
    std::string path;

    int version;
};

struct InstalledVersion {
    std::string name;
    std::string path;

    int version;
};

class VersionManager {
public:
    VersionManager(std::string folderName);

    bool FetchVersions(const std::wstring& url, const std::wstring& path);
    const std::vector<RemoteVersion>& GetRemoteVersions() const;

    const std::vector<InstalledVersion> GetInstalledVersions();

    void FinishVersionInstall(std::string selectedName, DownloadTask* task);

    std::filesystem::path GetVersionsFolder();
private:
    std::string rootFolderName;
    std::vector<RemoteVersion> remoteVersions;

    void ParseVersions(const std::string& data);
};