#include "HTTPSHelper.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <winhttp.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

HttpsResponse HTTPSHelper::SendBasicRequest(const std::wstring& address, const std::wstring& path, int port, const wchar_t* method, const std::string& requestData, const std::wstring contentType, bool secure) {
	std::vector<std::wstring> headers;
	headers.push_back(L"Content-Type: " + contentType);

	return HTTPSHelper::SendAdvancedRequest(address, path, port, method, requestData, headers, secure);
}

HttpsResponse HTTPSHelper::SendAdvancedRequest(const std::wstring& address, const std::wstring& path, int port, const wchar_t* method, const std::string& requestData, const std::vector<std::wstring>& headers, bool secure) {
	HttpsResponse response;
	response.status = -1;

	HINTERNET hSession = WinHttpOpen(L"MCLEMP-Launcher", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return response;

	HINTERNET hConnect = WinHttpConnect(hSession, address.c_str(), port, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return response; }

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, method, path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, (secure ? WINHTTP_FLAG_SECURE : 0));
	if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return response; }

	for (const auto& header : headers) {
		WinHttpAddRequestHeaders(hRequest, header.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
	}

	BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)requestData.c_str(), (DWORD)requestData.size(), (DWORD)requestData.size(), 0);
	if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

	if (bResults) {
		DWORD dwStatusCode = 0;
		DWORD dwSize = sizeof(dwStatusCode);
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
		response.status = dwStatusCode;

		DWORD dwDownloaded = 0;
		do {
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
			if (dwSize == 0) break;
			char* pszOutBuffer = new char[dwSize + 1];
			if (!pszOutBuffer) break;
			ZeroMemory(pszOutBuffer, dwSize + 1);
			if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
				response.body.append(pszOutBuffer, dwDownloaded);
			}
			delete[] pszOutBuffer;
		} while (dwSize > 0);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}