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

static constexpr ImGuiID GLOBAL_POPUP_ID = 0xBEEFAB1E;

#define BEGIN_GLOBAL_POPUP() \
    do { \
        ImGui::PushOverrideID(GLOBAL_POPUP_ID); \
    } while (0)

#define END_GLOBAL_POPUP() \
    do { \
        ImGui::PopID(); \
    } while (0)

#define OPEN_GLOBAL_POPUP(popupId) \
    do { \
        ImGui::PushOverrideID(GLOBAL_POPUP_ID); \
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
    bool getArrangementMode() const {
        SessionManager& sessionManager = SessionManager::getInstance();

        if (sessionManager.getSessionAvaliable())
            return sessionManager.getCurrentSession()->arrangementMode;
        else
            return false;
    }

    bool focusOnSelectedPart { false };
};

#endif // APPSTATE_HPP
