#include "LCEOnlineServices.h"
#include "HTTPSHelper.h"
#include <sstream>
#include <vector>
#include <string>
#include <thread>

std::vector<std::string> split(const std::string& str, char delimiter);

LCEOnlineServices::AccountInformation LCEOnlineServices::accountInfoCache;
LCEOnlineServices::SocialLists LCEOnlineServices::socialCache = LCEOnlineServices::SocialLists(true, "");

bool LCEOnlineServices::offlineMode = false;

bool LCEOnlineServices::DoesHaveAccount(Config* configSystem) {
	if (LCEOnlineServices::offlineMode && !LCEOnlineServices::accountInfoCache.GetUsername().empty()) return true;

	std::map<std::string, std::string> credsMap = configSystem->ReadConfigFile(configSystem->credentialsPath);

	if (credsMap.find("token") != credsMap.end() && !credsMap["token"].empty()) { return true; }
	return false;
}

bool LCEOnlineServices::isAccountValid(Config* configSystem) {
	if (LCEOnlineServices::offlineMode && !LCEOnlineServices::accountInfoCache.GetUsername().empty()) return true;

	accountInfoCache = LCEOnlineServices::GetAccountInformation(configSystem);

	return !accountInfoCache.GetUsername().empty();
}

void LCEOnlineServices::SaveAccountInformation(Config* configSystem) {
	std::map<std::string, std::string> credsMap = configSystem->ReadConfigFile(configSystem->credentialsPath);

	if (!(LCEOnlineServices::offlineMode && LCEOnlineServices::accountInfoCache.GetToken().empty())) {
		credsMap["token"] = LCEOnlineServices::accountInfoCache.GetToken();

	}
	credsMap["lastUsername"] = LCEOnlineServices::accountInfoCache.GetUsername();

	configSystem->WriteConfigFile(configSystem->credentialsPath, credsMap, credsMap); //its ok to use this as default map

}

static const wchar_t* ServerAddress = L"38.49.215.81";
static const int ServerPort = 3502;

LCEOnlineServices::AccountInformation LCEOnlineServices::GetAccountInformation(Config* configSystem) {
	std::map<std::string, std::string> credsMap = configSystem->ReadConfigFile(configSystem->credentialsPath);
	auto it = credsMap.find("token");

	if (it != credsMap.end()) {
		HttpsResponse response = HTTPSHelper::SendBasicRequest(ServerAddress, L"getAccountInfo", ServerPort, L"POST", it->second, L"text/plain", false);

		if (response.body[0] == '-') {
			return LCEOnlineServices::AccountInformation(it->second, response.body.substr(1, response.body.size()));
		}
	}
	return LCEOnlineServices::AccountInformation("", "");
}

void LCEOnlineServices::FetchSessionToken(std::function<void(LCEOnlineServices::APIResponse)> callback) {
	std::thread t([callback]() {
		HttpsResponse response = HTTPSHelper::SendBasicRequest(ServerAddress, L"getSessionTicket", ServerPort, L"POST", LCEOnlineServices::accountInfoCache.GetToken(), L"text/plain", false);
		OutputDebugStringA(response.body.c_str());
		callback(APIResponse(response.body[0] != '-', response.body.substr(1, response.body.size())));
	});

	t.detach();
}

void LCEOnlineServices::GetSocialLists(std::function<void(LCEOnlineServices::SocialLists)> callback) {
	std::thread t([callback]() {
		HttpsResponse response = HTTPSHelper::SendBasicRequest(
			ServerAddress,
			L"getSocialLists",
			ServerPort,
			L"POST",
			LCEOnlineServices::accountInfoCache.GetToken(),
			L"text/plain",
			false
		);

		if (response.body[0] == '-') {
			std::string data = response.body.substr(1);
			std::vector<std::string> sections = split(data, '|');

			std::vector<std::string> friends;
			std::vector<std::string> requests;
			std::vector<std::string> blocked;

			if (sections.size() >= 1 && !sections[0].empty())
				friends = split(sections[0], ',');

			if (sections.size() >= 2 && !sections[1].empty())
				requests = split(sections[1], ',');

			if (sections.size() >= 3 && !sections[2].empty())
				blocked = split(sections[2], ',');

			callback(LCEOnlineServices::SocialLists(friends, requests, blocked));
		}
		else {
			callback(LCEOnlineServices::SocialLists(true, response.body));
		}
		});

	t.detach();
}

void LCEOnlineServices::SendFriendRequest(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback) {
	std::thread t([target, callback]() {
		std::string payload = LCEOnlineServices::accountInfoCache.GetToken() + ":" + target;

		HttpsResponse response = HTTPSHelper::SendBasicRequest(
			ServerAddress,
			L"sendFriendRequest",
			ServerPort,
			L"POST",
			payload,
			L"text/plain",
			false
		);

		callback(LCEOnlineServices::APIResponse(response.body != "1", response.body));
		});

	t.detach();
}

void LCEOnlineServices::AcceptFriendRequest(const std::string& from, std::function<void(LCEOnlineServices::APIResponse)> callback) {
	std::thread t([from, callback]() {
		std::string payload = LCEOnlineServices::accountInfoCache.GetToken() + ":" + from;

		HttpsResponse response = HTTPSHelper::SendBasicRequest(
			ServerAddress,
			L"acceptFriendRequest",
			ServerPort,
			L"POST",
			payload,
			L"text/plain",
			false
		);

		callback(LCEOnlineServices::APIResponse(response.body != "1", response.body));
		});

	t.detach();
}

void LCEOnlineServices::RemoveFriendRequest(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback) {
	std::thread t([target, callback]() {
		std::string payload = LCEOnlineServices::accountInfoCache.GetToken() + ":" + target;

		HttpsResponse response = HTTPSHelper::SendBasicRequest(
			ServerAddress,
			L"removeFriend",
			ServerPort,
			L"POST",
			payload,
			L"text/plain",
			false
		);

		callback(LCEOnlineServices::APIResponse(response.body != "1", response.body));
	});

	t.detach();
}

void LCEOnlineServices::BlockUser(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback) {
	std::thread t([target, callback]() {
		std::string payload = LCEOnlineServices::accountInfoCache.GetToken() + ":" + target;

		HttpsResponse response = HTTPSHelper::SendBasicRequest(
			ServerAddress,
			L"blockUser",
			ServerPort,
			L"POST",
			payload,
			L"text/plain",
			false
		);

		callback(LCEOnlineServices::APIResponse(response.body != "1", response.body));
	});

	t.detach();
}

void LCEOnlineServices::UnblockUser(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback) {
	std::thread t([target, callback]() {
		std::string payload = LCEOnlineServices::accountInfoCache.GetToken() + ":" + target;

		HttpsResponse response = HTTPSHelper::SendBasicRequest(
			ServerAddress,
			L"unblockUser",
			ServerPort,
			L"POST",
			payload,
			L"text/plain",
			false
		);

		callback(LCEOnlineServices::APIResponse(response.body != "1", response.body));
	});

	t.detach();
}

void LCEOnlineServices::AttemptAccountLogin(const std::string username, const std::string password, std::function<void(LCEOnlineServices::LoginResponse)> callback) {
	std::thread t([username, password, callback]() {

		HttpsResponse response = HTTPSHelper::SendBasicRequest(ServerAddress, L"accountLogin", ServerPort, L"POST", std::string(username + ":" + password), L"text/plain", false);

		if (response.body[0] == '-') {
			std::string parsedString = response.body.substr(1, response.body.size());
			std::vector<std::string> responseData = split(parsedString, ':');

			LCEOnlineServices::accountInfoCache = AccountInformation(responseData[1], responseData[0]);

			callback(LoginResponse(false, ""));
		} else {
			callback(LoginResponse(true, response.body));
		}
	});

	t.detach();
}

void LCEOnlineServices::AttemptAccountRegistration(const std::string username, const std::string password, std::function<void(LCEOnlineServices::LoginResponse)> callback) {
	std::thread t([username, password, callback]() {
		HttpsResponse response = HTTPSHelper::SendBasicRequest(ServerAddress, L"accountRegistration", ServerPort, L"POST", std::string(username + ":" + password), L"text/plain", false);

		if (response.body[0] == '-') {
			std::string parsedString = response.body.substr(1, response.body.size());
			std::vector<std::string> responseData = split(parsedString, ':');

			LCEOnlineServices::accountInfoCache = AccountInformation(responseData[1], responseData[0]);

			callback(LoginResponse(false, ""));
		} else {
			callback(LoginResponse(true, response.body));
		}
	});

	t.detach();
}


std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;

	while (std::getline(ss, item, delimiter)) {
		result.push_back(item);
	}

	return result;
}