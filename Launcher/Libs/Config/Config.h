#pragma once
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

class Config {
public:
	Config(std::string folderName);

	std::map<std::string, std::string> ReadConfigFile(std::string& path);
	void WriteConfigFile(std::string& path, const std::map<std::string, std::string>& current, const std::map<std::string, std::string>& defaultMap);

	std::string configPath;
	std::string credentialsPath;
private:
	void SimpleMigrationAndCheck(std::string& path, std::map<std::string, std::string>& defaultMap);
};