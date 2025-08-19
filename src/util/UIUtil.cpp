#include "UIUtil.hpp"

#include <imgui_internal.h>

#include <stdexcept>

bool UIUtil::Widget::StdStringTextInput(const char* label, std::string& str) {
    constexpr ImGuiTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    return ImGui::InputText(label, str.data(), str.capacity() + 1, flags,
        [](ImGuiInputTextCallbackData* data) -> int {
            std::string* str = reinterpret_cast<std::string*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                if (data->Buf != str->data()) {
                    throw std::runtime_error("UIUtil::Widget::StdStringTextInput: data->Buf is not equal to str->data() ..");
                }

                if (str->size() != data->BufTextLen) {
                    str->resize(data->BufTextLen);
                    data->Buf = str->data();
                }
            }

            return 0;
        }, &str
    );
}

static ImU32 itemButtonColorU32() {
    const bool held    = ImGui::IsItemHovered() && ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered() && (ImGui::GetActiveID() !=  ImGui::GetCurrentWindowRead()->MoveId);

    const ImU32 color = ImGui::GetColorU32(
        (held & hovered) ? ImGuiCol_ButtonActive
        : hovered ? ImGuiCol_ButtonHovered
        : ImGuiCol_Button
    );

    return color;
}

bool UIUtil::Widget::SplitButton(const char *strId, const char *label, const ImVec2 &sizeArg) {
    if (ImGui::GetCurrentWindowRead()->SkipItems)
        return false;

    ImDrawList &drawList = *ImGui::GetCurrentWindow()->DrawList;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float fontSize    = ImGui::GetFontSize();
    const ImVec2 groupPos   = ImGui::GetCursorScreenPos();

    ImGui::BeginGroup();
    ImGui::PushID(strId);

    bool togglePressed = false;
    float buttonHeight = 0.0;
    {
        const ImVec2 pos     = ImGui::GetCursorScreenPos();
        const ImVec2 minSize = ImGui::CalcTextSize(label, NULL, true) + style.FramePadding * 2.0f;
        const ImVec2 size    = ImGui::CalcItemSize(sizeArg, minSize.x, minSize.y);
        buttonHeight = size.y;

        togglePressed = ImGui::InvisibleButton("toggle", size, ImGuiButtonFlags_EnableNav);

        const ImRect bb(pos, pos + size);
        drawList.AddRectFilled(
            bb.Min, bb.Max,
            itemButtonColorU32(),
            style.FrameRounding, ImDrawFlags_RoundCornersLeft
        );
        drawList.AddText(pos + style.FramePadding, ImGui::GetColorU32(ImGuiCol_Text), label);
    }

    bool popupOpen = ImGui::IsPopupOpen("");
    {
        ImGui::SameLine(0.0, 1.0);
        const ImVec2 pos  = ImGui::GetCursorScreenPos();
        const ImVec2 size = ImVec2(buttonHeight, buttonHeight);

        bool pressed = ImGui::InvisibleButton("dropdown", size, ImGuiButtonFlags_EnableNav);

        if (pressed && !popupOpen) {
            ImGui::OpenPopup("");
            popupOpen = true;
        }

        const ImRect bb(pos, pos + size);
        drawList.AddRectFilled(
            bb.Min, bb.Max,
            popupOpen ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : itemButtonColorU32(),
            style.FrameRounding, ImDrawFlags_RoundCornersRight
        );

        ImVec2 mid = bb.Min + ImVec2(
            ImMax(0.0f, (size.x - fontSize) * 0.5f),
            ImMax(0.0f, (size.y - fontSize) * 0.5f)
        );

        ImGui::RenderArrow(&drawList, mid, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Down);
    }

    ImGui::PopID();
    ImGui::EndGroup();
    ImVec2 groupSize = ImGui::GetItemRectSize();

    if (popupOpen) {
        char name[20];
        ImFormatString(name, IM_ARRAYSIZE(name), "##Popup_%08x", ImGui::GetCurrentWindow()->GetID(strId));

        if (ImGuiWindow *popupWindow = ImGui::FindWindowByName(name)) {
            const ImRect bb(groupPos, groupPos + groupSize);

            popupWindow->AutoPosLastDirection = ImGuiDir_Down;
            ImGui::SetNextWindowPos(ImGui::FindBestWindowPosForPopupEx(
                bb.GetBL(),
                ImGui::CalcWindowNextAutoFitSize(popupWindow),
                &popupWindow->AutoPosLastDirection,
                ImGui::GetPopupAllowedExtentRect(popupWindow),
                bb,
                ImGuiPopupPositionPolicy_ComboBox
            ));
        }
    }

    return togglePressed;
}
