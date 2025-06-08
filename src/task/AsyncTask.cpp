#include "AsyncTask.hpp"

#include <thread>

#include <imgui.h>
#include <imgui_internal.h>

#include "../font/FontAwesome.h"

#include "../Macro.hpp"

AsyncTask::AsyncTask(AsyncTaskId id, const char* message) :
    mId(id), mMessage(message)
{
    mStartTime = static_cast<float>(ImGui::GetTime());
};

void AsyncTask::Start() {
    mIsComplete = false;

    std::thread([this]() {
        Run();
        mIsComplete = true;
    }).detach();
}

void AsyncTask::RunEffectIfComplete() {
    if (mIsComplete && !mHasEffectRun) {
        Effect();
        mHasEffectRun = true;
    }
}

void AsyncTask::ShowPopup() const {
    ImGui::PushID(static_cast<int>(mId));

    if (!mIsComplete)
        ImGui::OpenPopup("###WORKING");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25.f, 20.f });

    CENTER_NEXT_WINDOW();
    if (ImGui::BeginPopupModal((const char*)ICON_FA_WAND_MAGIC_SPARKLES "  Toasting ..###WORKING", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(mMessage);

        ImGui::Dummy({ 0.f, 1.f });

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGui::ProgressBar(
            -1.f * (static_cast<float>(ImGui::GetTime()) - mStartTime),
            { 0.f, 15.f }
        );
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    ImGui::PopID();
}
