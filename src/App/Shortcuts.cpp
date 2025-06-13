#include "Shortcuts.hpp"

#include <unordered_map>

#include <string>

#include <stdexcept>

#include "Actions.hpp"

#include "Logging.hpp"

#include "manager/SessionManager.hpp"

namespace Shortcuts {

namespace {

const std::unordered_map<ShortcutAction, Shortcut> ShortcutMap = {
    { ShortcutAction::Open, { "Open (szs, zlib)...", std::string(CTRL_SYMBOL) + "+O", ImGuiKey_O | ImGuiMod_Ctrl } },
    { ShortcutAction::SaveAs, { "Save as...", std::string(CTRL_SYMBOL) + "+Shift+S", ImGuiKey_S | ImGuiMod_Ctrl | ImGuiMod_Shift } },
    { ShortcutAction::Save, { "Save", std::string(CTRL_SYMBOL) + "+S", ImGuiKey_S | ImGuiMod_Ctrl } },
    { ShortcutAction::Redo, { "Redo", std::string(CTRL_SYMBOL) + "+Shift+Z", ImGuiKey_Z | ImGuiMod_Ctrl | ImGuiMod_Shift } },
    { ShortcutAction::Undo, { "Undo", std::string(CTRL_SYMBOL) + "+Z", ImGuiKey_Z | ImGuiMod_Ctrl } },
    { ShortcutAction::Exit, { "Exit", EXIT_SHORTCUT, ImGuiKey_None } }
};

const Shortcut* findShortcut(ShortcutAction action) {
    auto it = ShortcutMap.find(action);
    return (it != ShortcutMap.end()) ? &it->second : nullptr;
}

} // namespace

std::string getDisplayName(ShortcutAction action, const char8_t* icon) {
    const Shortcut* shortcut = findShortcut(action);
    if (!shortcut) {
        throw std::runtime_error("Shortcuts::getDisplayName: Shortcut not found (" + std::to_string(static_cast<int>(action)) + ").");
    }

    return icon ? (std::string(reinterpret_cast<const char*>(icon)) + " " + shortcut->displayName)
                : shortcut->displayName;
}

const std::string& getDisplayChord(ShortcutAction action) {
    const Shortcut* shortcut = findShortcut(action);
    if (!shortcut) {
        throw std::runtime_error("Shortcuts::getDisplayChord: Shortcut not found (" + std::to_string(static_cast<int>(action)) + ").");
    }
    return shortcut->displayChord;
}

void Process() {
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
