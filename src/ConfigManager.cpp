#include "ConfigManager.hpp"

#include "Logging.hpp"

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
    std::lock_guard<std::mutex> lock(mMtx);

    std::ifstream file(mConfigPath);

    // Config doesn't exist, create default config.
    if (!file.is_open()) {
        mFirstTime = true;

        LoadDefaults();
        SaveConfig();

        return;
    }

    nlohmann::ordered_json j;
    file >> j;
    file.close();

    mConfig = j.get<Config>();

    clampRecentlyOpened(mConfig.recentlyOpened);
}

void ConfigManager::SaveConfig() const {
    std::ofstream file(mConfigPath);
    if (!file.is_open()) {
        Logging::err << "[ConfigManager::Save] Unable to open file for saving." << std::endl;
        return;
    }

    nlohmann::ordered_json j = mConfig;
    file << j.dump(4);

    file.close();
}

void ConfigManager::LoadDefaults() {
    mConfig = Config {};
}

void ConfigManager::addRecentlyOpened(const std::string_view path) {
    std::lock_guard<std::mutex> lock(mMtx);

    auto& recentlyOpened = mConfig.recentlyOpened;

    // Remove already existing instances of path
    recentlyOpened.erase(
        std::remove(recentlyOpened.begin(), recentlyOpened.end(), path),
        recentlyOpened.end()
    );

    recentlyOpened.push_back(std::string(path));

    clampRecentlyOpened(recentlyOpened);
}
