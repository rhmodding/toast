#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include "Singleton.hpp"

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>

using json = nlohmann::json;

// Stores instance of ConfigManager in local configManager.
#define GET_CONFIG_MANAGER ConfigManager& configManager = ConfigManager::getInstance()

class ConfigManager : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>; // Allow access to base class constructor

public:
    struct Config {
        bool darkTheme{ true };

        std::string imageEditorPath{ "" };

        std::string textureEditPath{ "./toastEditTexture.temp.png" };

        std::string lastFileDialogPath{ "." };

        bool showUnknownValues{ false };

        int lastWindowWidth{ 1500 };
        int lastWindowHeight{ 780 };

        float updateRate{ 120 };

        bool operator==(const Config& other) const {
            return
                this->darkTheme == other.darkTheme &&
                this->imageEditorPath == other.imageEditorPath &&
                this->textureEditPath == other.textureEditPath &&
                this->lastFileDialogPath == other.lastFileDialogPath &&
                this->showUnknownValues == other.showUnknownValues &&
                this->lastWindowWidth == other.lastWindowWidth &&
                this->lastWindowHeight == other.lastWindowHeight &&
                this->updateRate == other.updateRate;
        }
    } config;

    bool firstTime{ false };
    std::string configPath{ "toast.config.json" };

    void LoadConfig() {
        std::ifstream file(configPath);

        if (!file.is_open()) {
            firstTime = true;

            this->ResetConfig();
            this->SaveConfig();

            return;
        }

        json data = json::parse(file);
        file.close();

        this->config.darkTheme = data["theme"] != "light";

        this->config.imageEditorPath = data["imageEditorPath"];
        this->config.textureEditPath = data["textureEditPath"];
        this->config.lastFileDialogPath = data["lastFileDialogPath"];

        this->config.showUnknownValues = data["showUnknownValues"];

        this->config.lastWindowHeight = data["lastWindowHeight"];
        this->config.lastWindowWidth = data["lastWindowWidth"];

        this->config.updateRate = data["updateRate"];
    }

    void SaveConfig() {
        std::ofstream file(this->configPath);

        if (!file.is_open()) {
            std::cerr << "[ConfigManager::SaveConfig] Unable to open file for saving.\n";
            return;
        }

        json data;

        data["theme"] = this->config.darkTheme ? "dark" : "light";

        data["imageEditorPath"] = this->config.imageEditorPath;
        data["textureEditPath"] = this->config.textureEditPath;
        data["lastFileDialogPath"] = this->config.lastFileDialogPath;

        data["showUnknownValues"] = this->config.showUnknownValues;

        data["lastWindowHeight"] = this->config.lastWindowHeight;
        data["lastWindowWidth"] = this->config.lastWindowWidth;

        data["updateRate"] = this->config.updateRate;

        file << std::setw(4) << data << std::endl; // Pretty print JSON with indentation
        file.close();
    }

    void ResetConfig() {
        config = Config{};
    }

private:
    ConfigManager() {} // Private constructor to prevent instantiation
};

#endif // CONFIGMANAGER_HPP