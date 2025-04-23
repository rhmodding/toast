#ifndef SHORTCUTS_HPP
#define SHORTCUTS_HPP

#include "Actions.hpp"

#include "../Logging.hpp"

#include <unordered_map>

#include <string>

#include <stdexcept>

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

inline const std::unordered_map<ShortcutAction, Shortcut> ShortcutMap = {
    { ShortcutAction::Open, { "Open (szs, zlib)...", std::string(CTRL_SYMBOL) + "+O", ImGuiKey_O | ImGuiMod_Ctrl } },
    { ShortcutAction::SaveAs, { "Save as...", std::string(CTRL_SYMBOL) + "+Shift+S", ImGuiKey_S | ImGuiMod_Ctrl | ImGuiMod_Shift } },
    { ShortcutAction::Save, { "Save", std::string(CTRL_SYMBOL) + "+S", ImGuiKey_S | ImGuiMod_Ctrl } },
    { ShortcutAction::Redo, { "Redo", std::string(CTRL_SYMBOL) + "+Shift+Z", ImGuiKey_Z | ImGuiMod_Ctrl | ImGuiMod_Shift } },
    { ShortcutAction::Undo, { "Undo", std::string(CTRL_SYMBOL) + "+Z", ImGuiKey_Z | ImGuiMod_Ctrl } },
    { ShortcutAction::Exit, { "Exit", EXIT_SHORTCUT, ImGuiKey_None }} // Just for show; we don't actually handle it
};

[[nodiscard]] std::string getDisplayName(ShortcutAction action, const char8_t* icon = nullptr) {
    auto it = ShortcutMap.find(action);
    if (it == ShortcutMap.end()) {
        throw std::runtime_error(
            "Shortcuts::getDisplayName: Shortcut not found (" + std::to_string(static_cast<int>(action)) + ")."
        );
    }

    const Shortcut& shortcut = it->second;

    if (icon)
        return std::string((const char*)icon) + " " + shortcut.displayName;
    else
        return shortcut.displayName;
}

[[nodiscard]] const std::string& getDisplayChord(ShortcutAction action) {
    auto it = ShortcutMap.find(action);
    if (it == ShortcutMap.end()) {
        throw std::runtime_error(
            "Shortcuts::getDisplayChord: Shortcut not found (" + std::to_string(static_cast<int>(action)) + ")."
        );
    }

    const Shortcut& shortcut = it->second;

    return shortcut.displayChord;
}

inline void Handle() {
    SessionManager& sessionManager = SessionManager::getInstance();

    for (const auto& [action, shortcut] : ShortcutMap) {
        if (shortcut.chord == ImGuiKey_None)
            continue;

        ImGuiInputFlags flags = ImGuiInputFlags_RouteAlways;
        if (action == ShortcutAction::Undo || action == ShortcutAction::Redo)
            flags |= ImGuiInputFlags_Repeat;

        if (ImGui::Shortcut(shortcut.chord, flags)) {
            switch (action) {
                case ShortcutAction::Open:
                    return Actions::CreateSessionPromptPath();
                case ShortcutAction::Save:
                    return Actions::ExportSession();
                case ShortcutAction::SaveAs:
                    return Actions::ExportSessionPromptPath();
                case ShortcutAction::Undo:
                    if (sessionManager.getSessionAvaliable())
                        return sessionManager.getCurrentSession()->undo();
                    break;
                case ShortcutAction::Redo:
                    if (sessionManager.getSessionAvaliable())
                        return sessionManager.getCurrentSession()->redo();
                    break;

                default:
                    break;
            }
        }
    }
}

} // namespace Shortcuts

#endif // SHORTCUTS_HPP
