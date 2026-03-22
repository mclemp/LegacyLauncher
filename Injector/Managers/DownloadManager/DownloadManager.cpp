#include "DownloadManager.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <fstream>
#include <vector>
#include <thread>

#pragma comment(lib, "winhttp.lib")

bool DownloadManager::DownloadFileSync(const std::wstring& host, const std::wstring& path, int port, const std::wstring& outputFile, DownloadTask* task) {
    HINTERNET hSession = WinHttpOpen(L"MyDownloader", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    OutputDebugStringW(std::wstring(host + L"\n").c_str());
    OutputDebugStringW(std::wstring(path + L"\n").c_str());


    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) return false;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    if (!hRequest) return false;

    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

    if (!bResults) return false;

    DWORD size = sizeof(DWORD);
    DWORD contentLength = 0;

    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &size, WINHTTP_NO_HEADER_INDEX)) {
        task->total.store(contentLength);
    }

    task->total = contentLength;

    std::ofstream file(outputFile, std::ios::binary);
    if (!file.is_open()) return false;

    DWORD dwSize = 0;

    do {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;

        std::vector<char> buffer(dwSize);

        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) break;

        file.write(buffer.data(), dwDownloaded);

        task->downloaded.fetch_add(dwDownloaded, std::memory_order_relaxed);

        //std::this_thread::sleep_for(std::chrono::microseconds(1));
    } while (dwSize > 0);

    file.close();

    task->success = true;
    task->done = true;

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}