#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <string>
#include <winhttp.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

struct HttpsResponse {
	int status;
	std::string body;
};

class HTTPSHelper {
public:
	static HttpsResponse SendBasicRequest(const std::wstring& address, const std::wstring& path, int port, const wchar_t* method, const std::string& requestData, const std::wstring contentType = L"text/plain");
	static HttpsResponse SendAdvancedRequest(const std::wstring& address, const std::wstring& path, int port, const wchar_t* method, const std::string& requestData, const std::vector<std::wstring>& headers);
};

