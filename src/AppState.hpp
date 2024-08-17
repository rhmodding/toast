#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "anim/Animatable.hpp"
#include "anim/RvlCellAnim.hpp"

#include "SessionManager.hpp"
#include "ConfigManager.hpp"

#include <algorithm>

#include <cstdint>
#include <cstring>

#include <imgui.h>
#include <imgui_internal.h>

#include "common.hpp"

// Stores instance of AppState in local appState.
#define GET_APP_STATE AppState& appState = AppState::getInstance()

// Stores globalAnimatable refpointer in local globalAnimatable.
#define GET_ANIMATABLE Animatable*& globalAnimatable = AppState::getInstance().globalAnimatable

#define BEGIN_GLOBAL_POPUP ImGui::PushOverrideID(AppState::getInstance().globalPopupID)
#define END_GLOBAL_POPUP ImGui::PopID()

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

private:
    ImVec4 windowClearColor{ Common::RGBAtoImVec4(24, 24, 24, 255) };
public:
    bool enableDemoWindow{ false };

    inline bool getDarkThemeEnabled() {
        return ConfigManager::getInstance().getConfig().theme == ThemeChoice_Dark;
    }

    void applyTheming() {
        if (this->getDarkThemeEnabled()) {
            ImGui::StyleColorsDark();
            this->windowClearColor = Common::RGBAtoImVec4(24, 24, 24, 255);
        }
        else {
            ImGui::StyleColorsLight();
            this->windowClearColor = Common::RGBAtoImVec4(248, 248, 248, 255);
        }

        ImGui::GetStyle().Colors[ImGuiCol_TabSelectedOverline] = ImVec4();
        ImGui::GetStyle().Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4();
    }

    inline ImVec4 getWindowClearColor() {
        return this->windowClearColor;
    }

    inline uint32_t getUpdateRate() {
        return ConfigManager::getInstance().getConfig().updateRate;
    }

    ImGuiID globalPopupID{ ImHashStr("GlobalPopupID") };
    inline void OpenGlobalPopup(const char* popupId) {
        ImGui::PushOverrideID(this->globalPopupID);
        ImGui::OpenPopup(popupId);
        ImGui::PopID();
    }

    struct Fonts {
        ImFont* normal;
        ImFont* large;
        ImFont* giant;
        ImFont* icon;
    } fonts;

    bool getArrangementMode() {
        if (LIKELY(SessionManager::getInstance().currentSession >= 0))
            return SessionManager::getInstance().getCurrentSession()->arrangementMode;
        else
            return false;
    }

    // This key object is meant for the arrangement mode to control the state of the key.
    RvlCellAnim::AnimationKey controlKey{ 0, 1, 1.f, 1.f, 0.f, 255 };

    uint16_t selectedAnimation{ 0 };
    int32_t selectedPart{ -1 };

    int32_t getMatchingNamePartIndex(
        RvlCellAnim::ArrangementPart& part,
        RvlCellAnim::Arrangement& arrangement
    ) {
        if (part.editorName[0] == '\0')
            return -1;

        for (unsigned i = 0; i < arrangement.parts.size(); i++) {
            const auto& lPart = arrangement.parts.at(i);
            if (
                lPart.editorName[0] != '\0' &&
                (strncmp(lPart.editorName, part.editorName, 32) == 0)
            )
                return i;
        }

        return -1;
    }

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
