#ifndef POPUP_SPRITESHEETMANAGER_HPP
#define POPUP_SPRITESHEETMANAGER_HPP

#include <imgui.h>

#include "manager/SessionManager.hpp"

#include "Macro.hpp"

static void Popup_SpritesheetManager() {
    static unsigned selectedSheet { 0 };

    SessionManager& sessionManager = SessionManager::getInstance();
    if (sessionManager.getCurrentSession() == nullptr)
        return;

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    bool shouldStayOpen = true;
    if (ImGui::BeginPopupModal("Spritesheet Manager###SpritesheetManager", &shouldStayOpen)) {
        if (ImGui::IsWindowAppearing())
            selectedSheet = sessionManager.getCurrentSession()
                ->getCurrentCellAnim().object->getSheetIndex();

        if (shouldStayOpen == false)
            ImGui::CloseCurrentPopup();

        auto& sheets = sessionManager.getCurrentSession()->sheets;

        // Left
        {
            ImGui::BeginChild("SheetList", { 150.f, 0.f }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
            for (unsigned i = 0; i < sheets->getTextureCount(); i++) {
                char label[32];
                std::snprintf(label, sizeof(label), "Sheet no. %u", i + 1);
                if (ImGui::Selectable(label, selectedSheet == i))
                    selectedSheet = i;
            }
            ImGui::EndChild();
        }

        const std::shared_ptr cellanimSheet = sheets->getTextureByIndex(selectedSheet);

        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();

            ImGui::SetNextWindowSizeConstraints(
                { 256.f, 0.f },
                {
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()
                }
            );
            ImGui::BeginChild("Preview", { 0.f, -ImGui::GetFrameHeightWithSpacing() });

            static float bgScale { .215f };

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##BGSlider", &bgScale, 0.f, 1.f, "BG: %.3f");

            ImVec2 imageSize {
                static_cast<float>(cellanimSheet->getWidth()),
                static_cast<float>(cellanimSheet->getHeight())
            };

            float imageScale = std::min(
                ImGui::GetContentRegionAvail().x / imageSize.x, // width ratio
                ImGui::GetContentRegionAvail().y / imageSize.y // height ratio
            );
            imageSize.x *= imageScale;
            imageSize.y *= imageScale;

            const ImVec2 imageTL {
                ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - imageSize.x) * .5f,
                ImGui::GetCursorScreenPos().y + (ImGui::GetContentRegionAvail().y - imageSize.y) * .5f
            };
            const ImVec2 imageBR { imageTL.x + imageSize.x, imageTL.y + imageSize.y, };

            ImGui::GetWindowDrawList()->AddRectFilled(
                imageTL, imageBR,
                ImGui::ColorConvertFloat4ToU32({ bgScale, bgScale, bgScale, 1.f })
            );

            ImGui::GetWindowDrawList()->AddImage(
                cellanimSheet->getImTextureId(),
                imageTL, imageBR
            );

            ImGui::EndChild();

            ImGui::EndGroup();
        }

        ImGui::EndPopup();
    }
}

#endif // POPUP_SPRITESHEETMANAGER_HPP
