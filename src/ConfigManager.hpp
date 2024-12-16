#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include "Singleton.hpp"

#include <cstdint>

#include <string>

#include <vector>

#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#include "common.hpp"

// Stores instance of ConfigManager in local configManager.
#define GET_CONFIG_MANAGER ConfigManager& configManager = ConfigManager::getInstance()

enum BackupBehaviour {
    BackupBehaviour_None,
    BackupBehaviour_SaveOnce,
    BackupBehaviour_SaveEverytime
};

NLOHMANN_JSON_SERIALIZE_ENUM(BackupBehaviour, {
    {BackupBehaviour_None, "None"},
    {BackupBehaviour_SaveOnce, "SaveOnce"},
    {BackupBehaviour_SaveEverytime, "SaveEverytime"}
});

enum ThemeChoice : int {
    ThemeChoice_Light,
    ThemeChoice_Dark
};

NLOHMANN_JSON_SERIALIZE_ENUM(ThemeChoice, {
    {ThemeChoice_Light, "Light"},
    {ThemeChoice_Dark, "Dark"},
});

class ConfigManager : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>; // Allow access to base class constructor

public:
    struct Config {
        ThemeChoice theme { ThemeChoice_Dark };

        std::string imageEditorPath { "" };

        std::string textureEditPath { "./toastEditTexture.temp.png" };

        std::vector<std::string> recentlyOpened;

        int lastWindowWidth { 1500 };
        int lastWindowHeight { 780 };

        bool canvasLMBPanEnabled { true };

        unsigned updateRate { 120 };

        BackupBehaviour backupBehaviour { BackupBehaviour_None };

        unsigned compressionLevel { 9 };

        bool operator==(const Config& other) const {
            return
                this->theme == other.theme &&
                this->imageEditorPath == other.imageEditorPath &&
                this->textureEditPath == other.textureEditPath &&
                this->lastWindowWidth == other.lastWindowWidth &&
                this->lastWindowHeight == other.lastWindowHeight &&
                this->canvasLMBPanEnabled == other.canvasLMBPanEnabled &&
                this->updateRate == other.updateRate &&
                this->backupBehaviour == other.backupBehaviour &&
                this->compressionLevel == other.compressionLevel;
        }
    };
private:
    Config config;

public:
    const Config& getConfig() const {
        return config;
    }
    void setConfig(const Config& newConfig) {
        config = newConfig;
    }

    // Friend functions for JSON (de-)serialization
    friend void to_json(nlohmann::ordered_json& j, const ConfigManager::Config& _config) {
        j = nlohmann::ordered_json {
            { "theme", _config.theme },
            { "imageEditorPath", _config.imageEditorPath },
            { "textureEditPath", _config.textureEditPath },
            { "recentlyOpened", _config.recentlyOpened },
            { "lastWindowWidth", _config.lastWindowWidth },
            { "lastWindowHeight", _config.lastWindowHeight },
            { "canvasLMBPanEnabled", _config.canvasLMBPanEnabled },
            { "updateRate", _config.updateRate },
            { "backupBehaviour", _config.backupBehaviour },
            { "compressionLevel", _config.compressionLevel }
        };
    }
    friend void from_json(const nlohmann::ordered_json& j, ConfigManager::Config& _config) {
        _config.theme =               j.value("theme", _config.theme);
        _config.imageEditorPath =     j.value("imageEditorPath", _config.imageEditorPath);
        _config.textureEditPath =     j.value("textureEditPath", _config.textureEditPath);
        _config.recentlyOpened =      j.value("recentlyOpened", _config.recentlyOpened);
        _config.lastWindowWidth =     j.value("lastWindowWidth", _config.lastWindowWidth);
        _config.lastWindowHeight =    j.value("lastWindowHeight", _config.lastWindowHeight);
        _config.canvasLMBPanEnabled = j.value("canvasLMBPanEnabled", _config.canvasLMBPanEnabled);
        _config.updateRate =          j.value("updateRate", _config.updateRate);
        _config.backupBehaviour =     j.value("backupBehaviour", _config.backupBehaviour);
        _config.compressionLevel =    j.value("compressionLevel", _config.compressionLevel);
    }

public:
    bool firstTime { false };
    std::string configPath { "toast.config.json" };

    void Load() {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            this->firstTime = true;

            this->Reset();
            this->Save();

            return;
        }

        nlohmann::ordered_json j;
        file >> j;
        file.close();

        this->config = j.get<Config>();
    }

    void Save() {
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

    void Reset() {
        this->config = Config {};
    }

private:
    ConfigManager() {} // Private constructor to prevent instantiation
};

#endif // CONFIGMANAGER_HPP
