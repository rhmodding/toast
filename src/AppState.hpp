#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "anim/Animatable.hpp"

#include "SessionManager.hpp"

#include "ConfigManager.hpp"

#include <cstdint>

#include <imgui.h>
#include <imgui_internal.h>

#include <chrono>

// Stores instance of AppState in local appState.
#define GET_APP_STATE AppState& appState = AppState::getInstance()

// Stores globalAnimatable refpointer in local globalAnimatable.
#define GET_ANIMATABLE Animatable*& globalAnimatable = AppState::getInstance().globalAnimatable

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

private:
    ImVec4 windowClearColor{ Common::RGBAtoImVec4(24, 24, 24, 255) };
public:
    bool enableDemoWindow{ false };

    inline bool getDarkThemeEnabled() {
        return ConfigManager::getInstance().getConfig().theme == ThemeChoice_Dark;
    }
    void UpdateTheme();

    inline ImVec4 getWindowClearColor() {
        return this->windowClearColor;
    }

    inline uint32_t getUpdateRate() {
        return ConfigManager::getInstance().getConfig().updateRate;
    }

    ImGuiID globalPopupID{ ImHashStr("GlobalPopupID") };

    struct Fonts {
        ImFont* normal;
        ImFont* large;
        ImFont* giant;
        ImFont* icon;
    } fonts;

    bool getArrangementMode() {
        if (SessionManager::getInstance().currentSession >= 0)
            return SessionManager::getInstance().getCurrentSession()->arrangementMode;
        else
            return false;
    }

    // This key object is meant for the arrangement mode to control the state of the key.
    RvlCellAnim::AnimationKey controlKey{ 0, 1, 1.f, 1.f, 0.f, 255 };

    uint16_t selectedAnimation{ 0 };
    int32_t selectedPart{ -1 };

    inline void correctSelectedPart() {
        this->selectedPart = std::clamp<int32_t>(
            this->selectedPart,
            -1,
            this->globalAnimatable->getCurrentArrangement()->parts.size() - 1
        );
    }

    bool focusOnSelectedPart{ false };

    Animatable* globalAnimatable{ nullptr };

    struct OnionSkinState {
        bool enabled{ false };

        uint16_t backCount{ 3 };
        uint16_t frontCount{ 2 };
        uint8_t opacity{ 50 };

        bool drawUnder{ false };
    } onionSkinState;

    enum CopyPasteContext {
        CopyPasteContext_None,

        CopyPasteContext_Animation,
        CopyPasteContext_Arrangement,
        CopyPasteContext_Part,
        CopyPasteContext_Key,

        CopyPasteContext_Max
    } lastCopyPasteContext{ CopyPasteContext_None };
private:
    AppState() {} // Private constructor to prevent instantiation
};

#endif // APPSTATE_HPP