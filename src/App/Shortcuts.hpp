#ifndef SHORTCUTS_HPP
#define SHORTCUTS_HPP

#include <string>

#include <imgui.h>

namespace Shortcuts {

#if defined(__APPLE__)
constexpr const char* CTRL_SYMBOL = "Cmd";
constexpr const char* EXIT_SHORTCUT = "Cmd+Q";
#else
constexpr const char* CTRL_SYMBOL = "Ctrl";
constexpr const char* EXIT_SHORTCUT = "Alt+F4";
#endif

enum class ShortcutAction {
    Open,
    Save,
    SaveAs,
    Undo,
    Redo,
    Exit
};

struct Shortcut {
    std::string displayName;
    std::string displayChord;
    ImGuiKeyChord chord;
};

[[nodiscard]] std::string getDisplayName(ShortcutAction action, const char8_t* icon = nullptr);
[[nodiscard]] const std::string& getDisplayChord(ShortcutAction action);

void process();

} // namespace Shortcuts

#endif // SHORTCUTS_HPP
