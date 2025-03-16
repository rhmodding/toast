#include "AsyncTask.hpp"

#include <thread>

#include <imgui.h>
#include <imgui_internal.h>

#include "../font/FontAwesome.h"

#include "../common.hpp"

AsyncTask::AsyncTask(AsyncTaskId id, const char* message) :
    id(id), message(message)
{
    this->startTime = static_cast<float>(ImGui::GetTime());
};

void AsyncTask::Start() {
    this->isComplete = false;

    std::thread([this]() {
        this->Run();
        this->isComplete = true;
    }).detach();
}

void AsyncTask::RunEffectIfComplete() {
    if (this->isComplete && !this->hasEffectRun) {
        this->Effect();
        this->hasEffectRun = true;
    }
}

void AsyncTask::ShowPopup() const {
    ImGui::PushID(this->id);

    if (!this->isComplete)
        ImGui::OpenPopup("###WORKING");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });

    CENTER_NEXT_WINDOW();
    if (ImGui::BeginPopupModal((const char*)ICON_FA_WAND_MAGIC_SPARKLES "  Toasting ..###WORKING", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(this->message);

        ImGui::Dummy({ 0.f, 1.f });

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGui::ProgressBar(
            -1.f * (static_cast<float>(ImGui::GetTime()) - this->startTime),
            { 0.f, 15.f }
        );
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    ImGui::PopID();
}
