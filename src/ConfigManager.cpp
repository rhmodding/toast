#include "ConfigManager.hpp"

#include <iostream>
#include <fstream>

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
}

void ConfigManager::SaveConfig() {
    std::ofstream file(this->configPath);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager::Save] Unable to open file for saving.\n";
        return;
    }

    auto& recentlyOpened = this->config.recentlyOpened;
    if (recentlyOpened.size() > 12) {
        recentlyOpened.erase(
            recentlyOpened.begin(),
            recentlyOpened.begin() + (recentlyOpened.size() - 12)
        );
    }

    nlohmann::ordered_json j = config;
    file << j.dump(4);

    file.close();
}

void ConfigManager::LoadDefaults() {
    this->config = Config {};
}
