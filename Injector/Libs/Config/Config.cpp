#include "Config.h"

std::string GetConfigPath(std::string name, std::string fileName = "config");
constexpr const char* CURRENT_VERSION = "1";
std::map<std::string, std::string> DEFAULT_CONFIG = {
    { "version", CURRENT_VERSION },
    { "key", "" },
};

std::map<std::string, std::string> CREDS_CONFIG = {
    { "version", CURRENT_VERSION },
    { "token", "" },
    { "lastUsername", "" },
};

void Config::SimpleMigrationAndCheck(std::string& path, std::map<std::string, std::string>& defaultMap) {
    std::map<std::string, std::string> config;

    if (std::filesystem::exists(path)) {
        config = ReadConfigFile(path);
        if (config["version"] != CURRENT_VERSION) {
            WriteConfigFile(path, config, defaultMap);
        }
    }
    else {
        WriteConfigFile(path, defaultMap, defaultMap);
    }
}

Config::Config(std::string folderName) {
    this->configPath = GetConfigPath(folderName);
    this->credentialsPath = GetConfigPath(folderName, "credentials");

    SimpleMigrationAndCheck(this->configPath, DEFAULT_CONFIG);
    SimpleMigrationAndCheck(this->credentialsPath, CREDS_CONFIG);
}



std::string GetConfigPath(std::string name, std::string fileName) {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::string full = std::string(path) + "\\" + name;
        std::filesystem::create_directories(full);
        return full + "\\" + fileName + ".inj";
    }
    return "";
}

std::map<std::string, std::string> Config::ReadConfigFile(std::string& path) {
    std::ifstream in(path);
    std::map<std::string, std::string> config;
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        config[key] = value;
    }

    return config;
}

void Config::WriteConfigFile(std::string& path, const std::map<std::string, std::string>& current, const std::map<std::string, std::string>& defaultMap) {
    std::ofstream out(path);
    out << "version=" << CURRENT_VERSION << "\n";

    for (const auto& [key, defaultVal] : defaultMap) {
        if (key == "version") continue;
        auto it = current.find(key);
        out << key << "=" << (it != current.end() ? it->second : defaultVal) << "\n";
    }

    for (const auto& [key, val] : current) {
        if (defaultMap.find(key) == defaultMap.end() && key != "version") {
            out << "# removed: " << key << "=" << val << "\n";
        }
    }
}
