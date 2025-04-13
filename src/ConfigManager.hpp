#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include "Singleton.hpp"

#include <string>

#include <vector>

#include <mutex>

#include <nlohmann/json.hpp>

enum BackupBehaviour {
    BackupBehaviour_None,
    BackupBehaviour_Save,
    BackupBehaviour_SaveOverwrite
};

NLOHMANN_JSON_SERIALIZE_ENUM(BackupBehaviour, {
    {BackupBehaviour_None, "None"},
    {BackupBehaviour_Save, "Save"},
    {BackupBehaviour_SaveOverwrite, "SaveOverwrite"}
});

enum ThemeChoice : int {
    ThemeChoice_Light,
    ThemeChoice_Dark
};

NLOHMANN_JSON_SERIALIZE_ENUM(ThemeChoice, {
    {ThemeChoice_Light, "Light"},
    {ThemeChoice_Dark, "Dark"},
});

enum ETC1Quality : int {
    ETC1Quality_Low,
    ETC1Quality_Medium,
    ETC1Quality_High,

    ETC1Quality_Count
};

NLOHMANN_JSON_SERIALIZE_ENUM(ETC1Quality, {
    {ETC1Quality_Low, "Low"},
    {ETC1Quality_Medium, "Medium"},
    {ETC1Quality_High, "High"},
});

struct Config {
public:
    static constexpr unsigned int MAX_RECENTLY_OPENED = 12;

    static constexpr unsigned int DEFAULT_WINDOW_WIDTH = 1280;
    static constexpr unsigned int DEFAULT_WINDOW_HEIGHT = 720;

public:
    ThemeChoice theme { ThemeChoice_Dark };

    std::string imageEditorPath { "" };

    std::string textureEditPath { "./toastEditTexture.temp.png" };

    std::vector<std::string> recentlyOpened;

    int lastWindowWidth { DEFAULT_WINDOW_WIDTH };
    int lastWindowHeight { DEFAULT_WINDOW_HEIGHT };

    bool lastWindowMaximized { false };

    bool canvasLMBPanEnabled { true };

    unsigned updateRate { 120 };

    BackupBehaviour backupBehaviour { BackupBehaviour_Save };

    unsigned compressionLevel { 9 };

    ETC1Quality etc1Quality { ETC1Quality_Medium };

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
            this->compressionLevel == other.compressionLevel &&
            this->etc1Quality == other.etc1Quality;
    }

    // Friend functions for JSON (de-)serialization
    friend void to_json(nlohmann::ordered_json& j, const Config& _config) {
        j = nlohmann::ordered_json {
            { "theme", _config.theme },
            { "imageEditorPath", _config.imageEditorPath },
            { "textureEditPath", _config.textureEditPath },
            { "recentlyOpened", _config.recentlyOpened },
            { "lastWindowWidth", _config.lastWindowWidth },
            { "lastWindowHeight", _config.lastWindowHeight },
            { "lastWindowMaximized", _config.lastWindowMaximized },
            { "canvasLMBPanEnabled", _config.canvasLMBPanEnabled },
            { "updateRate", _config.updateRate },
            { "backupBehaviour", _config.backupBehaviour },
            { "compressionLevel", _config.compressionLevel },
            { "etc1Quality", _config.etc1Quality }
        };
    }
    friend void from_json(const nlohmann::ordered_json& j, Config& _config) {
        _config.theme =               j.value("theme", _config.theme);
        _config.imageEditorPath =     j.value("imageEditorPath", _config.imageEditorPath);
        _config.textureEditPath =     j.value("textureEditPath", _config.textureEditPath);
        _config.recentlyOpened =      j.value("recentlyOpened", _config.recentlyOpened);
        _config.lastWindowWidth =     j.value("lastWindowWidth", _config.lastWindowWidth);
        _config.lastWindowHeight =    j.value("lastWindowHeight", _config.lastWindowHeight);
        _config.lastWindowMaximized = j.value("lastWindowMaximized", _config.lastWindowMaximized);
        _config.canvasLMBPanEnabled = j.value("canvasLMBPanEnabled", _config.canvasLMBPanEnabled);
        _config.updateRate =          j.value("updateRate", _config.updateRate);
        _config.backupBehaviour =     j.value("backupBehaviour", _config.backupBehaviour);
        _config.compressionLevel =    j.value("compressionLevel", _config.compressionLevel);
        _config.etc1Quality =         j.value("etc1Quality", _config.etc1Quality);
    }
};

class ConfigManager : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>; // Allow access to base class constructor

private:
    ConfigManager() = default;
public:
    ~ConfigManager() = default;

public:

public:
    const Config& getConfig() const { return this->config; }
    void setConfig(const Config& newConfig) {
        std::lock_guard<std::mutex> lock(this->mtx);
        config = newConfig;
    }

    void LoadConfig();
    void SaveConfig() const;

    void LoadDefaults();

    void addRecentlyOpened(const std::string_view path);

private:
    Config config {};

    bool firstTime { false };
    std::string configPath { "toast.config.json" };

    std::mutex mtx;
};

#endif // CONFIGMANAGER_HPP
