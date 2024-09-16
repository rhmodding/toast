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
    std::array<float, 4> windowClearColor;
public:
    inline bool getDarkThemeEnabled() {
        return ConfigManager::getInstance().getConfig().theme == ThemeChoice_Dark;
    }

    void applyTheming() {
        if (this->getDarkThemeEnabled()) {
            ImGui::StyleColorsDark();
            this->windowClearColor = std::array<float, 4>{ 24 / 255.f, 24 / 255.f, 24 / 255.f, 1.f };
        }
        else {
            ImGui::StyleColorsLight();
            this->windowClearColor = std::array<float, 4>{ 248 / 255.f, 248 / 255.f, 248 / 255.f, 1.f };
        }

        ImGui::GetStyle().Colors[ImGuiCol_TabSelectedOverline] = ImVec4();
        ImGui::GetStyle().Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4();
    }

    inline const std::array<float, 4>& getWindowClearColor() {
        return this->windowClearColor;
    }

    inline unsigned getUpdateRate() {
        return ConfigManager::getInstance().getConfig().updateRate;
    }

    ImGuiID globalPopupID{ 0xBEEFBEEF }; // BEEEEEEFFFF
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
    RvlCellAnim::AnimationKey controlKey;

    unsigned selectedAnimation{ 0 };
    int selectedPart{ -1 };

    int getMatchingNamePartIndex(
        RvlCellAnim::ArrangementPart& part,
        const RvlCellAnim::Arrangement& arrangement
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

    int getMatchingRegionPartIndex(
        RvlCellAnim::ArrangementPart& part,
        const RvlCellAnim::Arrangement& arrangement
    ) {
        for (unsigned i = 0; i < arrangement.parts.size(); i++) {
            const auto& lPart = arrangement.parts.at(i);
            if (
                lPart.regionX == part.regionX &&
                lPart.regionY == part.regionY &&
                lPart.regionW == part.regionW &&
                lPart.regionH == part.regionH
            )
                return i;
        }

        return -1;
    }

    inline void correctSelectedPart() {
        this->selectedPart = std::clamp<int>(
            this->selectedPart,
            -1,
            this->globalAnimatable->getCurrentArrangement()->parts.size() - 1
        );
    }

    bool focusOnSelectedPart{ false };

    Animatable* globalAnimatable{ nullptr };

    struct OnionSkinState {
        bool enabled{ false };
        bool drawUnder{ false };

        unsigned backCount{ 3 };
        unsigned frontCount{ 2 };

        uint8_t opacity{ 50 };
        uint8_t _pad24[3];
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
    ~AppState() {
        Common::deleteIfNotNullptr(this->globalAnimatable, false);
    }
};

#endif // APPSTATE_HPP
