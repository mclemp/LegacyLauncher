#include "LCEOnlineServices.h"
#include "HTTPHelper.h"
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

LCEOnlineServices::APIResponse LCEOnlineServices::AttemptAccountLogin(const std::string username, const std::string password) {
	HttpResponse response = HTTPHelper::SendBasicRequest(L"auth.goonchamber.gay", L"accountLogin", 443, L"POST", std::string(username + ":" + password));

	if (response.body[0] == '-') {
		//std::vector<std::string> responseData = 
	}
	OutputDebugString(response.body.c_str());


	return APIResponse(false, "s");
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