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

        std::string lastFileDialogPath{ "." };

        bool operator==(const Config& other) {
            return
                this->darkTheme == other.darkTheme &&
                this->imageEditorPath == other.imageEditorPath &&
                this->lastFileDialogPath == other.lastFileDialogPath;
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
        
        this->config.lastFileDialogPath = data["lastFileDialogPath"];

        std::cout << this->config.imageEditorPath << "\n";
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
        data["lastFileDialogPath"] = this->config.lastFileDialogPath;

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