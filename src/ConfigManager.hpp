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

        bool showUnknownValues{ false };

        int lastWindowWidth{ 1500 };
        int lastWindowHeight{ 780 };

        bool canvasLMBPanEnabled{ true };

        uint32_t updateRate{ 120 };

        BackupBehaviour backupBehaviour{ BackupBehaviour_None };

        bool operator==(const Config& other) const {
            return
                this->theme == other.theme &&
                this->imageEditorPath == other.imageEditorPath &&
                this->textureEditPath == other.textureEditPath &&
                this->showUnknownValues == other.showUnknownValues &&
                this->lastWindowWidth == other.lastWindowWidth &&
                this->lastWindowHeight == other.lastWindowHeight &&
                this->canvasLMBPanEnabled == other.canvasLMBPanEnabled &&
                this->updateRate == other.updateRate &&
                this->backupBehaviour == other.backupBehaviour;
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
            {"showUnknownValues", config.showUnknownValues},
            {"lastWindowWidth", config.lastWindowWidth},
            {"lastWindowHeight", config.lastWindowHeight},
            {"canvasLMBPanEnabled", config.canvasLMBPanEnabled},
            {"updateRate", config.updateRate},
            {"backupBehaviour", config.backupBehaviour}
        };
    }
    friend void from_json(const nlohmann::json& j, ConfigManager::Config& config) {
        j.at("theme").get_to(config.theme);
        j.at("imageEditorPath").get_to(config.imageEditorPath);
        j.at("textureEditPath").get_to(config.textureEditPath);
        j.at("showUnknownValues").get_to(config.showUnknownValues);
        j.at("lastWindowWidth").get_to(config.lastWindowWidth);
        j.at("lastWindowHeight").get_to(config.lastWindowHeight);
        j.at("canvasLMBPanEnabled").get_to(config.canvasLMBPanEnabled);
        j.at("updateRate").get_to(config.updateRate);
        j.at("backupBehaviour").get_to(config.backupBehaviour);
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
