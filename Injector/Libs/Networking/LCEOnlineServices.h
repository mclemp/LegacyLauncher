#pragma once
#include <string>

class LCEOnlineServices {
public:
	class APIResponse;

	static bool DoesHaveAccount();
	static bool isAccountValid();


	static LCEOnlineServices::APIResponse GenerateSessionTicket();
	static LCEOnlineServices::APIResponse AttemptAccountLogin(const std::string username, const std::string password);
	static LCEOnlineServices::APIResponse AttemptAccountRegistration(const std::string username, const std::string password);


	class APIResponse {
	public:
		bool wasError;
		std::string data;

		std::string& GetData() { return this->data; }

		APIResponse(bool _wasError, std::string _data) : wasError(_wasError), data(_data) {}
	};

	//class LoginResponse : public APIResponse {
	//public:
	//	LoginResponse(std::string accountToken) : APIResponse(false, accountToken) {}
	//	LoginResponse(bool _wasError, std::string errorString) : APIResponse(_wasError, errorString) {}
	//};

private:
};

