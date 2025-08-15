#ifndef POPUP_HPP
#define POPUP_HPP

#include <imgui.h>
#include <imgui_internal.h>

#include "../PopupHandler.hpp"

class Popup {
public:
    virtual void update() = 0;
    void open() {
        ImGui::PushOverrideID(Popups::GLOBAL_POPUP_ID);
        ImGui::OpenPopup(strId());
        ImGui::PopID();
    }
protected:
    Popup() = default;
    virtual const char* strId() = 0;
};

#endif