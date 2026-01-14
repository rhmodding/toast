#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include "Singleton.hpp"

#include <string>

#include <vector>

#include <mutex>

#include <nlohmann/json.hpp>

enum class BackupBehaviour {
    None,
    Save,
    SaveOverwrite
};

NLOHMANN_JSON_SERIALIZE_ENUM(BackupBehaviour, {
    {BackupBehaviour::None, "None"},
    {BackupBehaviour::Save, "Save"},
    {BackupBehaviour::SaveOverwrite, "SaveOverwrite"}
});

enum class ThemeChoice {
    Light,
    Dark,

    Count
};

NLOHMANN_JSON_SERIALIZE_ENUM(ThemeChoice, {
    {ThemeChoice::Light, "Light"},
    {ThemeChoice::Dark, "Dark"},
});

enum class ETC1Quality {
    Low,
    Medium,
    High,

    Count
};

NLOHMANN_JSON_SERIALIZE_ENUM(ETC1Quality, {
    {ETC1Quality::Low, "Low"},
    {ETC1Quality::Medium, "Medium"},
    {ETC1Quality::High, "High"},
});

struct Config {
public:
    static constexpr unsigned int MAX_RECENTLY_OPENED = 12;

    static constexpr unsigned int DEFAULT_WINDOW_WIDTH = 1280;
    static constexpr unsigned int DEFAULT_WINDOW_HEIGHT = 720;

public:
    ThemeChoice theme { ThemeChoice::Dark };

    std::string imageEditorPath { "" };

    std::string textureEditPath { "./toastEditTexture.temp.png" };

    std::vector<std::string> recentlyOpened;

    int lastWindowWidth { DEFAULT_WINDOW_WIDTH };
    int lastWindowHeight { DEFAULT_WINDOW_HEIGHT };

    bool lastWindowMaximized { false };

    bool canvasLMBPanEnabled { true };

    unsigned updateRate { 120 };

    BackupBehaviour backupBehaviour { BackupBehaviour::Save };

    unsigned compressionLevel { 9 };

    ETC1Quality etc1Quality { ETC1Quality::Medium };

    bool allowNewAnimCreate { false };

    bool operator==(const Config& rhs) const {
        return
            theme == rhs.theme &&
            imageEditorPath == rhs.imageEditorPath &&
            textureEditPath == rhs.textureEditPath &&
            // recentlyOpened == rhs.recentlyOpened &&
            lastWindowWidth == rhs.lastWindowWidth &&
            lastWindowHeight == rhs.lastWindowHeight &&
            canvasLMBPanEnabled == rhs.canvasLMBPanEnabled &&
            updateRate == rhs.updateRate &&
            backupBehaviour == rhs.backupBehaviour &&
            compressionLevel == rhs.compressionLevel &&
            etc1Quality == rhs.etc1Quality &&
            allowNewAnimCreate == rhs.allowNewAnimCreate;
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
            { "etc1Quality", _config.etc1Quality },
            { "allowNewAnimCreate", _config.allowNewAnimCreate }
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
        _config.allowNewAnimCreate =  j.value("allowNewAnimCreate", _config.allowNewAnimCreate);
    }
};

class ConfigManager : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>;

private:
    ConfigManager() = default;
public:
    ~ConfigManager() = default;

public:
    const Config& getConfig() const { return mConfig; }
    void setConfig(const Config& newConfig) {
        std::lock_guard<std::mutex> lock(mMtx);
        mConfig = newConfig;
    }

    void loadConfig();
    void saveConfig() const;

    void loadDefault();

    void pushRecentlyOpened(const std::string_view path);

private:
    Config mConfig {};

    bool mFirstTime { false };
    std::string mConfigPath { "toast.config.json" };

    std::mutex mMtx;
};

#endif // CONFIG_MANAGER_HPP
