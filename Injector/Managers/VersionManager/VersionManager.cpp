#include "VersionManager.h"
#include "../../Libs/Networking/HTTPSHelper.h"

#include "../../Libs/MiniZ/miniz.h"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <thread>

VersionManager::VersionManager(std::string folderName) {
    this->rootFolderName = folderName;
    std::filesystem::create_directories(GetVersionsFolder());
}

std::filesystem::path VersionManager::GetVersionsFolder() {
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "APPDATA");

    std::filesystem::path path = std::string(appdata) + "\\" + this->rootFolderName + "\\versions";
    free(appdata);

    return path;
}

bool VersionManager::FetchVersions(const std::wstring& url, const std::wstring& path) {
    HttpsResponse res = HTTPSHelper::SendBasicRequest(url, path, 443, L"GET", "", L"text/plain");

    if (res.status != 200) return false;

    ParseVersions(res.body);
    return true;
}

void VersionManager::ParseVersions(const std::string& data) {
    remoteVersions.clear();

    std::istringstream stream(data);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        size_t first = line.find('|');
        size_t second = line.find('|', first + 1);
        size_t third = line.find('|', second + 1);

        if (first == std::string::npos || second == std::string::npos || third == std::string::npos)
            continue;

        RemoteVersion v;

        v.name = line.substr(0, first);
        v.host = line.substr(first + 1, second - (first + 1));
        v.path = line.substr(second + 1, third - (second + 1));

        std::string versionStr = line.substr(third + 1);

        try {
            v.version = std::stoi(versionStr);
        } catch (...) {
            v.version = 0;
        }

        remoteVersions.push_back(v);
    }
}

const std::vector<RemoteVersion>& VersionManager::GetRemoteVersions() const {
    return remoteVersions;
}

const std::vector<InstalledVersion> VersionManager::GetInstalledVersions() {
    std::vector<InstalledVersion> versions;

    std::filesystem::path base = GetVersionsFolder();

    if (!std::filesystem::exists(base)) {
        return versions;
    }

    for (const auto& entry : std::filesystem::directory_iterator(base)) {
        if (!entry.is_directory())
            continue;

        std::filesystem::path versionFolder = entry.path();
        if (versionFolder.filename() == ".tempDownload")
            continue;

        std::filesystem::path configPath = versionFolder / "version.cfg";

        if (!std::filesystem::exists(configPath))
            continue;

        InstalledVersion v;
        v.path = versionFolder.generic_string();

        std::ifstream file(configPath);
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);

            if (key == "name") v.name = value;
            else if (key == "version") v.version = std::stoi(value);
            //else if (key == "host") v.host = value;
            //else if (key == "path") v.remotePath = value;
        }

        // fallback if config missing name
        if (v.name.empty()) {
            v.name = versionFolder.filename().string();
        }

        versions.push_back(v);
    }

    return versions;
}
bool IsBlacklisted(const std::filesystem::path& relativePath) {
    static std::vector<std::string> blacklist = { "Windows64"};

    std::string pathStr = relativePath.generic_string();

    for (const auto& entry : blacklist) {
        if (entry.back() == '/') {
            if (pathStr.starts_with(entry)) return true;
        } else {
            if (pathStr == entry) return true;
        }
    }

    return false;
}
void VersionManager::FinishVersionInstall(std::string selectedName, DownloadTask* task) {
    std::thread([this, selectedName, task]() {
        std::vector<RemoteVersion> remoteVersions = GetRemoteVersions();

        task->success = false;
        task->done = false;
        task->total = 1;
        task->downloaded = 0;

        for (RemoteVersion version : remoteVersions) {
            if (version.name == selectedName) {
                std::filesystem::path baseFolder = GetVersionsFolder();
                std::filesystem::path tempFolder = baseFolder / ".tempDownload";
                std::filesystem::path zipPath = tempFolder / "download.zip";

                if (!std::filesystem::exists(zipPath)) {
                    OutputDebugStringA("ZIP NOT FOUND\n");
                    task->done = true;
                    return;
                }

                std::filesystem::path installPath = baseFolder / version.name;

                std::filesystem::create_directories(installPath);

                mz_zip_archive zip{};
                if (!mz_zip_reader_init_file(&zip, zipPath.string().c_str(), 0)) {
                    OutputDebugStringA("FAILED TO OPEN ZIP\n");
                    task->done = true;
                    return;
                }

                mz_uint fileCount = mz_zip_reader_get_num_files(&zip);
                task->total = fileCount;

                for (mz_uint i = 0; i < fileCount; i++) {
                    mz_zip_archive_file_stat fileStat;
                    mz_zip_reader_file_stat(&zip, i, &fileStat);

                    std::filesystem::path relativePath = fileStat.m_filename;
                    std::filesystem::path outPath = installPath / relativePath;

                    if (mz_zip_reader_is_file_a_directory(&zip, i)) {
                        std::filesystem::create_directories(outPath);
                    }
                    else {
                        if (IsBlacklisted(relativePath) && std::filesystem::exists(outPath)) {
                            OutputDebugStringA(("SKIPPED (blacklist): " + relativePath.string() + "\n").c_str());
                            continue;
                        }

                        std::filesystem::create_directories(outPath.parent_path());
                        if (!mz_zip_reader_extract_to_file(&zip, i, outPath.string().c_str(), 0)) {
                            OutputDebugStringA("FAILED EXTRACT FILE\n");
                        }
                    }
                    task->downloaded = i;
                }

                mz_zip_reader_end(&zip);

                std::filesystem::path configPath = installPath / "version.cfg";

                std::ofstream config(configPath);
                if (config.is_open()) {
                    config << "name=" << version.name << "\n";
                    config << "version=" << std::to_string(version.version) << "\n";
                    config << "host=" << version.host << "\n";
                    config << "path=" << version.path << "\n";
                    config.close();
                }

                std::filesystem::remove(zipPath);

                if (std::filesystem::is_empty(tempFolder)) {
                    std::filesystem::remove(tempFolder);
                }

                task->done = true;

                OutputDebugStringA("INSTALL COMPLETE\n");
                return;
            }
        }

        OutputDebugStringA("COULDNT FIND NAME IN REMOTE LIST\n");
    }).detach();
}
