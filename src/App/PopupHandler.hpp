#ifndef POPUPHANDLER_HPP
#define POPUPHANDLER_HPP

#include <imgui.h>

// secretly popupwork

namespace Popups {
    
void update();

void createSingletons();

void destroySingletons();

constexpr ImGuiID GLOBAL_POPUP_ID = 0xBEEFAB1E;

} // namespace Popups

#endif // POPUPHANDLER_HPP
