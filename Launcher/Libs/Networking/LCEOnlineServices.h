#pragma once
#include <string>
#include <functional>
#include "../Config/Config.h"

class LCEOnlineServices {
public:

	class APIResponse;
	class LoginResponse;
	class AccountInformation;

	class SocialLists;

	static LCEOnlineServices::AccountInformation accountInfoCache;

	static bool DoesHaveAccount(Config* configSystem);
	static bool isAccountValid(Config* configSystem);

	static AccountInformation GetAccountInformation(Config* configSystem);

	static void SaveAccountInformation(Config* configSystem);

	static void FetchSessionToken(std::function<void(LCEOnlineServices::APIResponse)> callback);

	static LCEOnlineServices::SocialLists socialCache;
	static void GetSocialLists(std::function<void(LCEOnlineServices::SocialLists)> callback);

	static void SendFriendRequest(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback);
	static void AcceptFriendRequest(const std::string& from, std::function<void(LCEOnlineServices::APIResponse)> callback);
	static void RemoveFriendRequest(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback);
	static void UnblockUser(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback);
	static void BlockUser(const std::string& target, std::function<void(LCEOnlineServices::APIResponse)> callback);

	static void AttemptAccountLogin(const std::string username, const std::string password, std::function<void(LCEOnlineServices::LoginResponse)> callback);
	static void AttemptAccountRegistration(const std::string username, const std::string password, std::function<void(LCEOnlineServices::LoginResponse)> callback);


	class APIResponse {
	public:
		bool wasError;
		std::string data;

		std::string& GetData() { return this->data; }

		APIResponse(bool _wasError, std::string _data) : wasError(_wasError), data(_data) {}
	};

	class SocialLists : public APIResponse {
	public:
		std::vector<std::string> friends;
		std::vector<std::string> requests;
		std::vector<std::string> blocked;

		SocialLists(std::vector<std::string> _friends, std::vector<std::string> _requests, std::vector<std::string> _blocked) : APIResponse(false, ""), friends(_friends), requests(_requests), blocked(_blocked) {};
		SocialLists(bool _wasError, std::string _data) : APIResponse(_wasError, _data) {}

		
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

	class AccountInformation {
	public:
		AccountInformation() : token(""), username("") {}
		AccountInformation(std::string _token, std::string _username) : token(_token), username(_username) {}

		std::string GetToken() { return this->token; }
		std::string GetUsername() { return this->username; }
	private:
		std::string token;
		std::string username;
	};

	static bool isOffline() { return offlineMode;  }
	static bool offlineMode;
};

