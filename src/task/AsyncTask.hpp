#ifndef ASYNCTASK_HPP
#define ASYNCTASK_HPP

#include <thread>
#include <atomic>

#include <imgui.h>
#include <imgui_internal.h>

#include "../font/FontAwesome.h"

#include "../common.hpp"

class AsyncTask {
public:
    AsyncTask(uint32_t id, const char* message) :
        id(id), isComplete(false),
        message(message)
    {
        char str[32];
        sprintf(str, "Task%u", id);

        this->imguiID = ImHashStr(str);

        this->startTime = static_cast<float>(ImGui::GetTime());
    }

    virtual ~AsyncTask() = default;

    void Start() {
        this->isComplete = false;

        std::thread([this]() {
            Run();
            this->isComplete = true;
        }).detach();
    }

    void RenderPopup() {
        ImGui::PushID(this->imguiID);

        if (LIKELY(!isComplete))
            ImGui::OpenPopup("###WORKING");

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });

        CENTER_NEXT_WINDOW;
        if (ImGui::BeginPopupModal((char*)ICON_FA_WAND_MAGIC_SPARKLES "  Working..###WORKING", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(this->message);

            ImGui::Dummy(ImVec2(0.f, 1.f));

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
            ImGui::ProgressBar(
                -1.f * static_cast<float>(ImGui::GetTime() - this->startTime),
                ImVec2(.0f, 15.f)
            );
            ImGui::PopStyleColor();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        ImGui::PopID();
    }

    void TryRunEffect() {
        if (UNLIKELY(this->isComplete) && !UNLIKELY(this->hasEffectRun)) {
            this->Effect();
            this->hasEffectRun = true;
        }
    }

    bool IsComplete() const { return isComplete; }
    bool HasEffectRun() const { return hasEffectRun; }

protected:
    virtual void Run() = 0;
    virtual void Effect() = 0;

private:
    uint32_t id;

    std::atomic<bool> isComplete;
    std::atomic<bool> hasEffectRun;

    const char* message;

    ImGuiID imguiID;

    float startTime;
};

#endif // ASYNCTASK_HPP
