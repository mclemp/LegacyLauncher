#pragma once
#include <string>

class LCEOnlineServices {
public:
	class APIResponse;
	class LoginResponse;

	static bool DoesHaveAccount();
	static bool isAccountValid();


	static LCEOnlineServices::APIResponse GenerateSessionTicket();
	static LCEOnlineServices::LoginResponse AttemptAccountLogin(const std::string username, const std::string password);
	static LCEOnlineServices::APIResponse AttemptAccountRegistration(const std::string username, const std::string password);


	class APIResponse {
	public:
		bool wasError;
		std::string data;

		std::string& GetData() { return this->data; }

		APIResponse(bool _wasError, std::string _data) : wasError(_wasError), data(_data) {}
	};

	class LoginResponse : public APIResponse {
	public:
		std::string token;
		std::string username;

		LoginResponse(std::string username, std::string token) : APIResponse(false, "") {
			this->token = token;
			this->username = username;
		}
		LoginResponse(bool _wasError, std::string errorString) : APIResponse(_wasError, errorString) {}
	};

private:
};

