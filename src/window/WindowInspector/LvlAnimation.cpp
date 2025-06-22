#include "../WindowInspector.hpp"

#include "manager/SessionManager.hpp"
#include "manager/ThemeManager.hpp"
#include "manager/PlayerManager.hpp"

#include "manager/AppState.hpp"

#include "command/CommandModifyAnimation.hpp"

#include "App/Popups.hpp"

#include "util/UIUtil.hpp"

#include "font/FontAwesome.h"

static std::string sanitizeAnimationName(std::string newName, bool isCtr) {
    if (newName.empty())
        return newName;
    
    if (!isCtr) {
        // Prefix an underscore if the first character is a digit.
        if (std::isdigit(newName[0])) {
            newName.insert(newName.begin(), '_');
        }

        // Replace invalid characters with underscores.
        for (char& c : newName) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                c = '_';
            }
        }
    }
    // On CTR we simply replace non-alnum and non-underscore characters.
    else {
        for (char& c : newName) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                c = '_';
            }
        }
    }

    return newName;
}

void WindowInspector::Level_Animation() {
    PlayerManager& playerManager = PlayerManager::getInstance();
    SessionManager& sessionManager = SessionManager::getInstance();

    drawPreview();

    const bool isCtr = sessionManager.getCurrentSession()->type == CellAnim::CELLANIM_TYPE_CTR;

    ImGui::SameLine();

    CellAnim::Animation& animation = playerManager.getAnimation();
    unsigned animationIndex = playerManager.getAnimationIndex();

    CellAnim::Animation newAnimation = playerManager.getAnimation();
    CellAnim::Animation originalAnimation = playerManager.getAnimation();

    ImGui::BeginChild("LevelHeader", { 0.f, 0.f }, ImGuiChildFlags_AutoResizeY);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

        ImGui::Text("Animation no. %u:", animationIndex + 1);

        ImGui::PushFont(ThemeManager::getInstance().getFonts().large);
        ImGui::TextWrapped("%s",
            animation.name.empty() ? "(no name set)" : animation.name.c_str()
        );
        ImGui::PopFont();

        ImGui::PopStyleVar();

        ImGui::EndChild();

    ImGui::SeparatorText((const char*)ICON_FA_PENCIL " Properties");

    if (isCtr) {
        ImGui::Checkbox("Interpolated", &newAnimation.isInterpolated);

        ImGui::Dummy({ 0.f, 5.f });
    }

    UIUtil::Widget::ValueEditor<std::string>("Name", animation.name,
        [&]() { return originalAnimation.name; },
        [&](const std::string& oldValue, const std::string& newValue) {
            originalAnimation.name = oldValue;
            newAnimation.name = sanitizeAnimationName(newValue, isCtr);
        },
        [](const char* label, std::string* value) {
            return UIUtil::Widget::StdStringTextInput(label, *value);
        }
    );

    if (!isCtr) {
        UIUtil::Widget::ValueEditor<std::string>("Comment", animation.comment,
            [&]() { return originalAnimation.comment; },
            [&](const std::string& oldValue, const std::string& newValue) {
                originalAnimation.comment = oldValue;
                newAnimation.comment = newValue;
            },
            [](const char* label, std::string* value) {
                return UIUtil::Widget::StdStringTextInput(label, *value);
            }
        );
    }

    ImGui::Dummy({ 0.f, 2.f });

    ImGui::Separator();

    ImGui::Dummy({ 0.f, 1.f });

    if (ImGui::Button("Swap animation..")) {
        Popups::_swapAnimationIdx = animationIndex;
        OPEN_GLOBAL_POPUP("###SwapAnimation");
    }

    if (!isCtr) {
        ImGui::Dummy({ 0.f, 1.f });

        ImGui::Separator();

        ImGui::Dummy({ 0.f, 2.f });

        ImGui::Indent();
            ImGui::Bullet(); ImGui::SameLine();
            ImGui::TextWrapped(
                "Animation names are not used to map animations in-game; instead, "
                "the animation index (its order in the list) determines mapping.\n"
                "To swap animations, use [Swap animation..]."
            );
        ImGui::Unindent();
    }

    if (newAnimation != originalAnimation) {
        animation = originalAnimation;

        sessionManager.getCurrentSession()->addCommand(
        std::make_shared<CommandModifyAnimation>(
            sessionManager.getCurrentSession()->getCurrentCellAnimIndex(),
            playerManager.getAnimationIndex(),
            newAnimation
        ));
    }
}
