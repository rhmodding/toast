#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include "Singleton.hpp"

#include <cstdint>

#include <string>

#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#include "common.hpp"

using json = nlohmann::json;

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

enum ThemeChoice {
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
        ThemeChoice theme{ ThemeChoice_Dark };

        std::string imageEditorPath{ "" };

        std::string textureEditPath{ "./toastEditTexture.temp.png" };
        
        int lastWindowWidth{ 1500 };
        int lastWindowHeight{ 780 };

        bool canvasLMBPanEnabled{ true };

        unsigned updateRate{ 120 };

        BackupBehaviour backupBehaviour{ BackupBehaviour_None };

        unsigned compressionLevel{ 8 };

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
    friend void to_json(nlohmann::json& j, const ConfigManager::Config& config) {
        j = nlohmann::json{
            {"theme", config.theme},
            {"imageEditorPath", config.imageEditorPath},
            {"textureEditPath", config.textureEditPath},
            {"lastWindowWidth", config.lastWindowWidth},
            {"lastWindowHeight", config.lastWindowHeight},
            {"canvasLMBPanEnabled", config.canvasLMBPanEnabled},
            {"updateRate", config.updateRate},
            {"backupBehaviour", config.backupBehaviour},
            {"compressionLevel", config.compressionLevel}
        };
    }
    friend void from_json(const nlohmann::json& j, ConfigManager::Config& config) {
        config.theme = j.value("theme", config.theme);
        config.imageEditorPath = j.value("imageEditorPath", config.imageEditorPath);
        config.textureEditPath = j.value("textureEditPath", config.textureEditPath);
        config.lastWindowWidth = j.value("lastWindowWidth", config.lastWindowWidth);
        config.lastWindowHeight = j.value("lastWindowHeight", config.lastWindowHeight);
        config.canvasLMBPanEnabled = j.value("canvasLMBPanEnabled", config.canvasLMBPanEnabled);
        config.updateRate = j.value("updateRate", config.updateRate);
        config.backupBehaviour = j.value("backupBehaviour", config.backupBehaviour);
        config.compressionLevel = j.value("compressionLevel", config.compressionLevel);
    }

public:
    bool firstTime{ false };
    std::string configPath{ "toast.config.json" };

    void Load() {
        std::ifstream file(configPath);
        if (UNLIKELY(!file.is_open())) {
            this->firstTime = true;

            this->Reset();
            this->Save();

            return;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        this->config = j.get<Config>();
    }

    void Save() const {
        std::ofstream file(this->configPath);
        if (UNLIKELY(!file.is_open())) {
            std::cerr << "[ConfigManager::Save] Unable to open file for saving.\n";
            return;
        }

        nlohmann::json j = config;
        file << j.dump(4);

        file.close();
    }

    void Reset() {
        config = Config{};
    }

private:
    ConfigManager() {} // Private constructor to prevent instantiation
};

#endif // CONFIGMANAGER_HPP
