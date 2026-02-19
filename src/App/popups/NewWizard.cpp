#include "NewWizard.hpp"

#include <tinyfiledialogs.h>

#include "util/UIUtil.hpp"

#include "font/FontAwesome.h"

#include "manager/SessionManager.hpp"

#include "App/Actions.hpp"

void Popups::NewWizard::update() {
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });
    if (ImGui::BeginPopupModal("Create a new project###NewWizard", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PopStyleVar();

        ImGui::Dummy({ 0.0f, 2.0f });

        ImGui::TextUnformatted("CellAnim Definition");

        size_t removeIndex = (size_t)-1;
        if (ImGui::BeginTable("CellAnimTable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            //ImGui::TableHeadersRow();

            for (size_t row = 0; row < mCellAnimAdd.size(); row++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::PushID(row);

                ImGui::TextUnformatted(mCellAnimAdd[row].name.c_str());

                ImGui::SameLine();

                const char *removeButtonText = (const char *)ICON_FA_CIRCLE_XMARK "##Remove";
                float padX = ImGui::GetContentRegionAvail().x -
                            ImGui::CalcTextSize(removeButtonText, nullptr, true).x;
                ImGui::Dummy({ padX, 0.0f });
                ImGui::SameLine(0.0f, 0.0f);

                if (UIUtil::Widget::LabelButton(removeButtonText, false)) {
                    removeIndex = row;
                }

                ImGui::PopID();
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            UIUtil::Widget::LabelButton("Add new ..", false);
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft)) {
                ImGui::TextUnformatted("Name");
                UIUtil::Widget::StdStringTextInput("###NameInput", mNewCellAnimEnt.name);

                ImGui::Separator();

                ImGui::BeginDisabled(mNewCellAnimEnt.name.empty());
                if (ImGui::Selectable("Add")) {
                    mCellAnimAdd.push_back(mNewCellAnimEnt);
                    mNewCellAnimEnt = CellAnimEnt {}; // Reset.
                }
                ImGui::EndDisabled();
                if (ImGui::Selectable("Cancel")) {
                    mNewCellAnimEnt = CellAnimEnt {}; // Reset.
                }

                ImGui::End();
            }

            ImGui::EndTable();
        }

        if (removeIndex != (size_t)-1) {
            mCellAnimAdd.erase(mCellAnimAdd.begin() + removeIndex);
        }

        ImGui::Dummy({ 0.0f, 2.0f });

        static const char * const targetNames[] = {
            /* TARGET_RVL */ "Rhythm Heaven Fever (RVL)",
            /* TARGET_CTR */ "Rhythm Heaven Megamix (CTR)"
        };
        ImGui::Combo("Target", &mSelectedTarget, targetNames, IM_ARRAYSIZE(targetNames));

        bool canCreate = !mCellAnimAdd.empty();

        ImGui::Dummy({ 0.0f, 5.0f });

        ImGui::BeginDisabled(!canCreate);

        if (ImGui::Button("Create", { 120.f, 0.f })) {
            const bool isRvl = mSelectedTarget == TARGET_RVL;

            const char *filterDesc = isRvl ? "RVL Cellanim Archive files" : "CTR Cellanim Archive files";
            const char *filterPatterns[] = { isRvl ? "*.szs" : "*.zlib" };

            char *savePath = tinyfd_saveFileDialog(
                "Select a file to save to",
                nullptr,
                ARRAY_LENGTH(filterPatterns), filterPatterns,
                filterDesc
            );

            if (savePath != nullptr) {
                SessionManager &sessionManager = SessionManager::getInstance();

                ssize_t sessionIndex = sessionManager.createSessionDefault(
                    isRvl ? CellAnim::CELLANIM_TYPE_RVL : CellAnim::CELLANIM_TYPE_CTR,
                    mCellAnimAdd.size()
                );
                if (sessionIndex >= 0) {
                    Session &session = sessionManager.getSession(sessionIndex);

                    for (size_t i = 0; i < mCellAnimAdd.size(); i++) {
                        session.getCellAnim(i).object->setName(mCellAnimAdd[i].name);
                        session.sheets->getTextureByIndex(i)->setName(mCellAnimAdd[i].name);
                    }
                    session.setResourcePath(std::string(savePath));

                    sessionManager.setCurrentSessionIndex(sessionIndex);
                    
                    Actions::ExportSession();
                }
            }

            ImGui::CloseCurrentPopup();
            resetFields();
        }
        ImGui::SetItemDefaultFocus();

        if (!canCreate && ImGui::BeginItemTooltip()) {
            if (mCellAnimAdd.empty()) {
                ImGui::BulletText("Please add at least one bank.");
            }

            ImGui::EndTooltip();
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Nevermind", { 120.f, 0.f })) {
            ImGui::CloseCurrentPopup();
            resetFields();
        }

        ImGui::EndPopup();
    }
    else {
        ImGui::PopStyleVar();
    }
}