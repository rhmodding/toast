#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "cellanim/CellAnim.hpp"

#include "SessionManager.hpp"

#include "PlayerManager.hpp"

#include "ConfigManager.hpp"

#include <algorithm>

#include <cstdint>

#include <imgui.h>
#include <imgui_internal.h>

#define BEGIN_GLOBAL_POPUP() \
    do { \
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID); \
    } while (0)

#define END_GLOBAL_POPUP() \
    do { \
        ImGui::PopID(); \
    } while (0)

#define OPEN_GLOBAL_POPUP(popupId) \
    do { \
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID); \
        ImGui::OpenPopup(popupId); \
        ImGui::PopID(); \
    } while (0)

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

private:
    AppState() = default;
public:
    ~AppState() = default;

public:
    unsigned getUpdateRate() const {
        return ConfigManager::getInstance().getConfig().updateRate;
    }

    ImGuiID globalPopupID { 0xBEEFAB1E };

    bool getArrangementMode() const {
        SessionManager& sessionManager = SessionManager::getInstance();

        if (sessionManager.getSessionAvaliable())
            return sessionManager.getCurrentSession()->arrangementMode;
        else
            return false;
    }

    bool focusOnSelectedPart { false };

    struct OnionSkinState {
        bool enabled { false };
        bool drawUnder { false };
        bool rollOver { false };

        unsigned backCount { 3 };
        unsigned frontCount { 2 };

        uint8_t opacity { 50 };
    } onionSkinState;
};

#endif // APPSTATE_HPP
