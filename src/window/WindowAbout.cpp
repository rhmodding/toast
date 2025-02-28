#include "WindowAbout.hpp"

#include <imgui.h>

#define _USE_MATH_DEFINES
#include <cmath>

#include <sstream>

#include <thread>

#include "../font/FontAwesome.h"

#include "../_binary/images/toastIcon_title.png.h"

#include "../AppState.hpp"

static const char* headerTitle = "toast";
static const char* headerDesc  =
    "the ultimate cellanim modding tool for\n"
    "Rhythm Heaven Fever / Beat the Beat: Rhythm Paradise";

static const char* githubButton  = (const char*)ICON_FA_CODE "";
static const char* githubTooltip = "Open GitHub page..";

static const char* githubURLCmd =
#ifdef _WIN32
    "start "
#elif __APPLE__
    "open "
#else
    "xdg-open "
#endif // __APPLE__, _WIN32
    "https://github.com/conhlee/toast";

enum CreditsLineCommand {
    ClcEmptyLine,
    ClcNextRow,
    ClcNextColumn,

    ClcStringLine,

    ClcSeparator
};
struct CreditsLine {
    CreditsLineCommand command;
    const char* string { nullptr };
};

static const CreditsLine creditStrings[] = {
    { ClcSeparator },

    { ClcStringLine, (const char*)ICON_FA_WRENCH "  Initial testing" },
    { ClcEmptyLine },

    { ClcStringLine, "Placeholder" },
    { ClcStringLine, "Placeholder Placeholder" },
    { ClcStringLine, "Placeholder" },
    { ClcStringLine, "Placeholder Placeholder" },

    { ClcNextRow },

    { ClcStringLine, (const char*)ICON_FA_FILE_CODE "  Open-source software used / referenced" },
    { ClcEmptyLine },

    { ClcStringLine, "Dear ImGui (ocornut/imgui) [MIT]" },
    { ClcStringLine, "GLFW [Zlib]" },
    { ClcStringLine, "tinyfiledialogs [Zlib]" },
    { ClcStringLine, "json (nlohmann/json) [MIT]" },
    { ClcStringLine, "zlib-ng [Zlib]" },
    { ClcStringLine, "syaz0 (zeldamods/syaz0) [GPL-2.0]" },

    { ClcNextColumn },

    { ClcStringLine, (const char*)ICON_FA_STAR "  Special thanks" },
    { ClcEmptyLine },
    { ClcStringLine, "Chrislo (a.k.a. chrislo27)" },
    { ClcStringLine, "patataofcourse" },
    { ClcStringLine, "TheAlternateDoctor" },
};

void WindowAbout::Update() {
    if (!this->open)
        return;

    if (this->image.getTextureId() == Texture::INVALID_TEXTURE_ID)
        image.LoadSTBMem(toastIcon_title_png, toastIcon_title_png_size);

    CENTER_NEXT_WINDOW();
    ImGui::SetNextWindowSize({ 820.f, 520.f });

    ImGui::Begin("About", &this->open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasBottomRight {
        canvasTopLeft.x + canvasSize.x,
        canvasTopLeft.y + canvasSize.y
    };

    const ImVec2 imageSize { 350.f, 350.f };

    const ImVec2 imageTopLeft {
        canvasTopLeft.x - 20.f,

        canvasTopLeft.y + ((canvasSize.y - imageSize.y) / 2) -
        ((cosf(ImGui::GetTime()) + 1) * 15.f) + 8.f
    };
    const ImVec2 imageBottomRight {
        imageTopLeft.x + imageSize.x,
        imageTopLeft.y + imageSize.y
    };

    drawList->AddImage((ImTextureID)this->image.getTextureId(), imageTopLeft, imageBottomRight);

    // Text
    ImVec2 textPosition {
        imageBottomRight.x + 10,
        canvasTopLeft.y + 20
    };

    // Set button position
    ImGui::SetCursorPos({
        window->WorkRect.Max.x - 25.f - 30.f - window->Pos.x,
        textPosition.y + (30.f / 2) - window->Pos.y
    });

    AppState& appState = AppState::getInstance();

    ImGui::PushFont(appState.fonts.giant);

    drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), headerTitle);
    textPosition.y += ImGui::GetTextLineHeight() + 5.f;

    ImGui::PopFont();

    drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), headerDesc);

    if (ImGui::Button(githubButton, { 30.f, 30.f })) {
        std::thread([]() {
            std::system(githubURLCmd);
        }).detach();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
        ImGui::SetTooltip("%s", githubTooltip);

    textPosition.y += (ImGui::GetTextLineHeight() * 2) + 5.f;

    ///////////////////////////////////////////////////////////////

    float additiveLineHeight { 0.f };
    float totalLineWidth { 0.f };
    float additiveLineX { 0.f };

    for (unsigned i = 0; i < ARRAY_LENGTH(creditStrings); i++) {
        switch (creditStrings[i].command) {
        case ClcSeparator: {
            textPosition.y += ImGui::GetTextLineHeight();

            const ImRect bounding(
                {
                    textPosition.x, textPosition.y + additiveLineHeight -
                    ( ImGui::GetTextLineHeight() / 2.f )
                },
                {
                    window->WorkRect.Max.x - 25.f, textPosition.y + additiveLineHeight -
                    ( ImGui::GetTextLineHeight() / 2.f ) + 1.f
                }
            );

            drawList->AddRectFilled(bounding.Min, bounding.Max, ImGui::GetColorU32(ImGuiCol_Separator));
        } continue;

        case ClcNextRow: {
            additiveLineX += totalLineWidth + 25.f;
            totalLineWidth = 0.f;

            additiveLineHeight = 0.f;
        } continue;

        case ClcNextColumn: {
            textPosition.y += additiveLineHeight + ImGui::GetTextLineHeight() + 6.f;
            additiveLineHeight = 0.f;

            totalLineWidth = 0.f;

            additiveLineX = 0.f;
        } continue;

        case ClcStringLine: {
            drawList->AddText(
                { textPosition.x + additiveLineX, textPosition.y + additiveLineHeight },
                ImGui::GetColorU32(ImGuiCol_Text), creditStrings[i].string
            );
        } break;

        default:
            break;
        }  

        additiveLineHeight += ImGui::GetTextLineHeight() + 2.f;

        totalLineWidth = std::max(totalLineWidth, ImGui::CalcTextSize(creditStrings[i].string).x);
    }

    ImGui::End();
}
