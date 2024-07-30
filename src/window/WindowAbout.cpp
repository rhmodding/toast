#include "WindowAbout.hpp"

#include <imgui.h>

#include "../_binary/images/toastIcon_title.png.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <sstream>

#include "../AppState.hpp"

#include "../font/FontAwesome.h"

const char* mainStrings[] = {
    "toast",
    "the ultimate cellanim modding tool for\nRhythm Heaven Fever / Beat the Beat: Rhythm Paradise",

    (char*)ICON_FA_CODE "",

    "https://github.com/conhlee/toast"
};

struct CreditsLine {
    enum CreditsLineCommand {
        CLC_EmptyLine,
        CLC_NextRow,
        CLC_NextColumn,

        CLC_DrawString,

        CLC_Separator,
    } command;

    const char* string{ nullptr };
} creditStrings[] = {
    { CreditsLine::CLC_Separator },

    { CreditsLine::CLC_DrawString, (char*)ICON_FA_WRENCH "  Testing" },
    { CreditsLine::CLC_EmptyLine },

    { CreditsLine::CLC_DrawString, "Placeholder" },
    { CreditsLine::CLC_DrawString, "Placeholder Placeholder" },
    { CreditsLine::CLC_DrawString, "Placeholder" },
    { CreditsLine::CLC_DrawString, "Placeholder Placeholder" },

    { CreditsLine::CLC_NextRow },

    { CreditsLine::CLC_DrawString, (char*)ICON_FA_FILE_CODE "  Open-source software used / referenced" },
    { CreditsLine::CLC_EmptyLine },

    { CreditsLine::CLC_DrawString, "Dear ImGui (ocornut/imgui) [MIT]" },
    { CreditsLine::CLC_DrawString, "GLFW [Zlib]" },
    { CreditsLine::CLC_DrawString, "tinyfiledialogs [Zlib]" },
    { CreditsLine::CLC_DrawString, "json (nlohmann/json) [MIT]" },
    { CreditsLine::CLC_DrawString, "zlib-ng [Zlib]" },
    { CreditsLine::CLC_DrawString, "syaz0 (zeldamods/syaz0) [GPL-2.0]" },

    { CreditsLine::CLC_NextColumn },

    { CreditsLine::CLC_DrawString, (char*)ICON_FA_STAR "  Special thanks" },
    { CreditsLine::CLC_EmptyLine },
    { CreditsLine::CLC_DrawString, "github:chrislo27" },
    { CreditsLine::CLC_DrawString, "github:patataofcourse" },
    { CreditsLine::CLC_DrawString, "github:TheAlternateDoctor" },
};

void WindowAbout::Update() {
    if (!this->open)
        return;

    static bool imageLoaded{ false };

    if (!imageLoaded) {
        image.LoadFromMem(toastIcon_title_png, toastIcon_title_png_size);
        imageLoaded = true;
    }

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::SetNextWindowSize({ 820, 520 });

    ImGui::Begin("About", &this->open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    GET_WINDOW_DRAWLIST;

    const ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasBottomRight = ImVec2(
        canvasTopLeft.x + canvasSize.x,
        canvasTopLeft.y + canvasSize.y
    );

    const ImVec2 imageSize = ImVec2(350, 350);

    const ImVec2 imageTopLeft = ImVec2(
        canvasTopLeft.x - 20,

        canvasTopLeft.y + ((canvasSize.y - imageSize.y) / 2) -
        ((cosf(ImGui::GetTime()) + 1) * 15) + 8
    );
    const ImVec2 imageBottomRight = ImVec2(
        imageTopLeft.x + imageSize.x,
        imageTopLeft.y + imageSize.y
    );

    drawList->AddImage((void*)(uintptr_t)this->image.texture, imageTopLeft, imageBottomRight);

    // Text
    ImVec2 textPosition = ImVec2(
        imageBottomRight.x + 10,
        canvasTopLeft.y + 20
    );

    // Set button position
    ImGui::SetCursorPos(ImVec2(
        window->WorkRect.Max.x - 25.f - 30.f - window->Pos.x,
        textPosition.y + (30.f / 2) - window->Pos.y
    ));

    GET_APP_STATE;

    ImGui::PushFont(appState.fonts.giant);

    drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), mainStrings[0]);
    textPosition.y += ImGui::GetTextLineHeight() + 5.f;

    ImGui::PopFont();

    drawList->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), mainStrings[1]);

    if (ImGui::Button(mainStrings[2], { 30.f, 30.f })) {
        std::ostringstream cmdStream;

        cmdStream <<
    #ifdef _WIN32
        "start ";
    #elif __APPLE__
        "open ";
    #else
        "xdg-open ";
    #endif

        cmdStream << mainStrings[3];

        std::system(cmdStream.str().c_str());
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
        ImGui::SetTooltip("Open GitHub page..");

    textPosition.y += (ImGui::GetTextLineHeight() * 2) + 5.f;

    ///////////////////////////////////////////////////////////////

    float additiveLineHeight{ 0 };
    float totalLineWidth{ 0 };
    float additiveLineX{ 0 };

    for (uint16_t i = 0; i < ARRAY_LENGTH(creditStrings); i++) {
        switch (creditStrings[i].command) {
            case CreditsLine::CLC_Separator: {
                textPosition.y += ImGui::GetTextLineHeight();

                const ImRect bounding(
                    ImVec2(
                        textPosition.x, textPosition.y + additiveLineHeight -
                        ( ImGui::GetTextLineHeight() / 2 )
                    ),
                    ImVec2(
                        window->WorkRect.Max.x - 25.f, textPosition.y + additiveLineHeight -
                        ( ImGui::GetTextLineHeight() / 2 ) + 1.f
                    )
                );

                drawList->AddRectFilled(bounding.Min, bounding.Max, ImGui::GetColorU32(ImGuiCol_Separator));
            } continue;

            case CreditsLine::CLC_NextRow:
                additiveLineX += totalLineWidth + 25.f;
                totalLineWidth = 0.f;

                additiveLineHeight = 0.f;
                continue;

            case CreditsLine::CLC_NextColumn:
                textPosition.y += additiveLineHeight + ImGui::GetTextLineHeight() + 6.f;
                additiveLineHeight = 0.f;

                totalLineWidth = 0.f;

                additiveLineX = 0.f;
                continue;

            default:
                break;
        }

        if (creditStrings[i].command == CreditsLine::CLC_DrawString)
            drawList->AddText(
                ImVec2(textPosition.x + additiveLineX, textPosition.y + additiveLineHeight),
                ImGui::GetColorU32(ImGuiCol_Text), creditStrings[i].string
            );

        additiveLineHeight += ImGui::GetTextLineHeight() + 2.f;

        totalLineWidth = std::max(totalLineWidth, ImGui::CalcTextSize(creditStrings[i].string).x);
    }

    ImGui::End();
}
