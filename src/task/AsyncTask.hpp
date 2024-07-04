#ifndef ASYNCTASK_HPP
#define ASYNCTASK_HPP

#include <thread>
#include <atomic>

#include <imgui.h>
#include <imgui_internal.h>

#include <sstream>

class AsyncTask {
public:
    AsyncTask(uint32_t id, const char* message) :
        id(id), isComplete(false),
        message(message)
    {
        char str[32];
        sprintf(str, "Task%u", id);

        this->imguiID = ImHashStr(str);
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

        if (!isComplete)
            ImGui::OpenPopup("Working..");

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 25, 20 });
        
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f));
        if (ImGui::BeginPopupModal("Working..", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(this->message);

            ImGui::Dummy(ImVec2(0.f, 1.f));

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
            ImGui::ProgressBar(-1.f * (float)ImGui::GetTime(), ImVec2(.0f, 15.f));
            ImGui::PopStyleColor();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        ImGui::PopID();
    }

    void TryRunEffect() {
        if (isComplete && !hasEffectRun) {
            Effect();
            hasEffectRun = true;
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
};

#endif // ASYNCTASK_HPP