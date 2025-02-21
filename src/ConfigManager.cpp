#include "ConfigManager.hpp"

#include <iostream>
#include <fstream>

inline void clampRecentlyOpened(std::vector<std::string>& recentlyOpened) {
    if (recentlyOpened.size() > Config::MAX_RECENTLY_OPENED) {
        recentlyOpened.erase(
            recentlyOpened.begin(),
            recentlyOpened.begin() + (recentlyOpened.size() - Config::MAX_RECENTLY_OPENED)
        );
    }
}

void ConfigManager::LoadConfig() {
    std::ifstream file(this->configPath);

    // Config doesn't exist, create default config.
    if (!file.is_open()) {
        this->firstTime = true;

        this->LoadDefaults();
        this->SaveConfig();

        return;
    }

    nlohmann::ordered_json j;
    file >> j;
    file.close();

    this->config = j.get<Config>();

    clampRecentlyOpened(this->config.recentlyOpened);
}

void ConfigManager::SaveConfig() const {
    std::ofstream file(this->configPath);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager::Save] Unable to open file for saving.\n";
        return;
    }

    nlohmann::ordered_json j = config;
    file << j.dump(4);

    file.close();
}

void ConfigManager::LoadDefaults() {
    this->config = Config {};
}

void ConfigManager::addRecentlyOpened(const std::string& path) {
    auto& recentlyOpened = this->config.recentlyOpened;

    // Remove already existing instances of path
    recentlyOpened.erase(
        std::remove(recentlyOpened.begin(), recentlyOpened.end(), path),
        recentlyOpened.end()
    );

    recentlyOpened.push_back(path);

    clampRecentlyOpened(recentlyOpened);
}
