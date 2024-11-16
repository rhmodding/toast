#include "WindowAbout.hpp"

#include <imgui.h>

#define _USE_MATH_DEFINES
#include <cmath>

#include <sstream>

#include "../font/FontAwesome.h"

#include "../_binary/images/toastIcon_title.png.h"

#include "../AppState.hpp"

const static char* headerTitle = "toast";
const static char* headerDesc  = "the ultimate cellanim modding tool for\nRhythm Heaven Fever / Beat the Beat: Rhythm Paradise";

const static char* githubButton  = (char*)ICON_FA_CODE "";
const static char* githubTooltip = "Open GitHub page..";
const static char* githubLink    = "https://github.com/conhlee/toast";

enum CreditsLineCommand {
    ClcEmptyLine,
    ClcNextRow,
    ClcNextColumn,

    ClcAddString,

    ClcSeparator
};
struct CreditsLine {
    CreditsLineCommand command;
    const char* string { nullptr };
};

static const CreditsLine creditStrings[] = {
    { ClcSeparator },

    { ClcAddString, (char*)ICON_FA_WRENCH "  Initial testing" },
    { ClcEmptyLine },

    { ClcAddString, "Placeholder" },
    { ClcAddString, "Placeholder Placeholder" },
    { ClcAddString, "Placeholder" },
    { ClcAddString, "Placeholder Placeholder" },

    { ClcNextRow },

    { ClcAddString, (char*)ICON_FA_FILE_CODE "  Open-source software used / referenced" },
    { ClcEmptyLine },

    { ClcAddString, "Dear ImGui (ocornut/imgui) [MIT]" },
    { ClcAddString, "GLFW [Zlib]" },
    { ClcAddString, "tinyfiledialogs [Zlib]" },
    { ClcAddString, "json (nlohmann/json) [MIT]" },
    { ClcAddString, "zlib-ng [Zlib]" },
    { ClcAddString, "syaz0 (zeldamods/syaz0) [GPL-2.0]" },

    { ClcNextColumn },

    { ClcAddString, (char*)ICON_FA_STAR "  Special thanks" },
    { ClcEmptyLine },
    { ClcAddString, "github:chrislo27" },
    { ClcAddString, "github:patataofcourse" },
    { ClcAddString, "github:TheAlternateDoctor" },
};

void WindowAbout::Update() {
    if (!this->open)
        return;

    static bool imageLoaded { false };

    if (!imageLoaded) {
        image.LoadSTBMem(toastIcon_title_png, toastIcon_title_png_size);
        imageLoaded = true;
    }

    CENTER_NEXT_WINDOW;
    ImGui::SetNextWindowSize({ 820.f, 520.f });

    ImGui::Begin("About", &this->open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    GET_WINDOW_DRAWLIST;

    const ImGuiWindow* window = ImGui::GetCurrentWindow();

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

    GET_APP_STATE;

    ImGui::PushFont(appState.fonts.giant);

    drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), headerTitle);
    textPosition.y += ImGui::GetTextLineHeight() + 5.f;

    ImGui::PopFont();

    drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), headerDesc);

    if (ImGui::Button(githubButton, { 30.f, 30.f })) {
        char buf[256];

        snprintf(buf, sizeof(buf), "%s %s",
    #ifdef _WIN32
            "start ",
    #elif __APPLE__
            "open ",
    #else
            "xdg-open ",
    #endif
            githubLink
        );

        std::system(buf);
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

            case ClcAddString: {
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
