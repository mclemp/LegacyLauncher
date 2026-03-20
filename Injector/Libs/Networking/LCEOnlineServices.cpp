#include "LCEOnlineServices.h"
#include "HTTPSHelper.h"
#include <sstream>
#include <vector>
#include <string>

std::vector<std::string> split(const std::string& str, char delimiter);

bool LCEOnlineServices::DoesHaveAccount() {
	return false;
}

bool LCEOnlineServices::isAccountValid() {
	return false;
}

LCEOnlineServices::LoginResponse LCEOnlineServices::AttemptAccountLogin(const std::string username, const std::string password) {
	HttpsResponse response = HTTPSHelper::SendBasicRequest(L"auth.goonchamber.gay", L"accountLogin", 443, L"POST", std::string(username + ":" + password));

	if (response.body[0] == '-') {
		std::string parsedString = response.body.substr(1, response.body.size());
		std::vector<std::string> responseData = split(parsedString, ':');

		return LoginResponse(responseData[0], responseData[1]);
	}


	return LoginResponse(true, response.body);
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