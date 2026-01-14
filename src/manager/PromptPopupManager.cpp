#include "PromptPopupManager.hpp"

#include <algorithm>

#include <cstdio>

#include <imgui.h>
#include <imgui_internal.h>

#include "util/CRC32Util.hpp"

#include "util/UIUtil.hpp"

uint32_t PromptPopupManager::sNextId { 0 };

struct ResponseEntry {
    PromptPopup::ResponseFlags flag;
    const char* label;
};

static const ResponseEntry sResponseEntries[] = {
    { PromptPopup::RESPONSE_OK,     "OK"     },
    { PromptPopup::RESPONSE_YES,    "Yes"    },
    { PromptPopup::RESPONSE_CANCEL, "Cancel" },
    { PromptPopup::RESPONSE_NO,     "No"     },
    { PromptPopup::RESPONSE_RETRY,  "Retry"  },
    { PromptPopup::RESPONSE_ABORT,  "Abort"  },
};

bool PromptPopup::show() {
    bool didShow = false;

    const std::string popupName = this->title + "###PromptPopup_" + std::to_string(this->id);

    ImGui::OpenPopup(popupName.c_str());

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f)
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 15.f });
    if (ImGui::BeginPopupModal(popupName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        didShow = true;

        ImGui::Dummy({ ImGui::CalcTextSize(this->title.c_str()).x - 30.f, 0.f });

        ImGui::TextUnformatted(this->message.c_str());

        if (this->hasInputField) {
            ImGui::Dummy({ 0.f, 2.f });
            UIUtil::Widget::StdStringTextInput("###InputField", this->currentInputText);
            ImGui::Dummy({ 0.f, 7.f });
            ImGui::Separator();
        }

        ImGui::Dummy({ 0.f, 5.f });

        bool isFirst = true;
        for (const auto& entry : sResponseEntries) {
            if (this->availableResponseFlags & entry.flag) {
                if (!isFirst) {
                    ImGui::SameLine();
                }
                else {
                    isFirst = false;
                }

                if (ImGui::Button(entry.label)) {
                    this->doCallback(static_cast<PromptPopup::Response>(entry.flag));
                    this->hasEnded = true;

                    ImGui::CloseCurrentPopup();
                }
                if (this->defaultResponse == entry.flag)
                    ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::Dummy({ 0.f, 1.f });

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    return didShow;
}

void PromptPopupManager::update() {
    queueFlush();

    ImGui::PushOverrideID(CRC32Util::compute("PromptPopups"));
    for (auto& popup : mActivePopups) {
        popup.show();
    }
    ImGui::PopID();

    mActivePopups.erase(
        std::remove_if(
            mActivePopups.begin(), mActivePopups.end(),
            [](const PromptPopup& p) { return p.hasEnded; }
        ),
        mActivePopups.end()
    );

    queueFlush();
}
