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

#define OPEN_GLOBAL_POPUP(popupId) \
    do { \
        ImGui::PushOverrideID(Popups::GLOBAL_POPUP_ID); \
        ImGui::OpenPopup(popupId); \
        ImGui::PopID(); \
    } while (0)

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>;

private:
    AppState() = default;
public:
    ~AppState() = default;

public:
    bool getArrangementMode() const {
        SessionManager& sessionManager = SessionManager::getInstance();

        if (sessionManager.isSessionAvailable())
            return sessionManager.getCurrentSession()->arrangementMode;
        else
            return false;
    }

    bool mFocusOnSelectedPart { false };
};

#endif // APPSTATE_HPP
