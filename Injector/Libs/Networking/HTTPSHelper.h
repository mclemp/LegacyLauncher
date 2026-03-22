#pragma once
#include <vector>
#include <string>


struct HttpsResponse {
	int status;
	std::string body;
};

class HTTPSHelper {
public:
	static HttpsResponse SendBasicRequest(const std::wstring& address, const std::wstring& path, int port, const wchar_t* method, const std::string& requestData, const std::wstring contentType = L"text/plain", bool secure = true);
	static HttpsResponse SendAdvancedRequest(const std::wstring& address, const std::wstring& path, int port, const wchar_t* method, const std::string& requestData, const std::vector<std::wstring>& headers, bool secure = true);
};

