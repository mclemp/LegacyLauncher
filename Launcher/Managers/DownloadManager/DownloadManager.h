#pragma once
#include <atomic>
#include <string>

struct DownloadTask {
    std::atomic<bool> done = false;
    std::atomic<bool> success = false;
    std::atomic<uint64_t> downloaded = 0;
    std::atomic<uint64_t> total = 0;
};

class DownloadManager {
public:
    static bool DownloadFileSync(const std::wstring& host, const std::wstring& path, int port, const std::wstring& outputFile, DownloadTask* task);
};

