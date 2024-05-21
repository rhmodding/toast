#include "WindowAbout.hpp"

#undef NDEBUG
#include "assert.h"

#include <imgui.h>

#include "../_binary/images/toastIcon_title.png.h"

#define _USE_MATH_DEFINES
#include <math.h>

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

    { CreditsLine::CLC_DrawString, "Testing" },
    { CreditsLine::CLC_EmptyLine },

    { CreditsLine::CLC_DrawString, "NAME               " },
    { CreditsLine::CLC_DrawString, "NAME               " },
    { CreditsLine::CLC_DrawString, "NAME               " },

    { CreditsLine::CLC_NextRow },

    { CreditsLine::CLC_DrawString, "Open-source software used / referenced" },
    { CreditsLine::CLC_EmptyLine },

    { CreditsLine::CLC_DrawString, "patataofcourse/flour" },
    { CreditsLine::CLC_DrawString, "rhmodding/bread" },
    { CreditsLine::CLC_DrawString, "KillzXGaming/Switch-Toolbox" },

    { CreditsLine::CLC_NextColumn },

    { CreditsLine::CLC_DrawString, "Yaz0 implementation" },
    { CreditsLine::CLC_EmptyLine },
    { CreditsLine::CLC_DrawString, "zeldamods/syaz0" },
};

WindowAbout::WindowAbout() {
    this->image.LoadFromMem(toastIcon_title_png, toastIcon_title_png_size);
}

inline float spinGraph(float t) {
    float period = 4.f;
    float holdTime = 1.f;

    float easeDuration = (period - 2 * holdTime) / 2.0;

    float holdEnd = easeDuration + holdTime;
    float downStart = holdEnd + easeDuration;

    // Reduce T to a single period using modulo operation
    float modT = fmodf(t, period);

    if (modT < easeDuration)
        return 1 / (1 + std::exp(-10 * ((modT / easeDuration) - 0.5f)));
    else if (modT < downStart)
        return 1 / (1 + std::exp(-10 * ((1.f - (modT - holdEnd) / easeDuration) - 0.5f)));
    else if (modT < holdEnd)
        return 1.f;
    
    return 0.f;
}

void WindowAbout::Update() {
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::SetNextWindowSize({ 820, 420 });

    ImGui::Begin("About", &this->open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    GET_WINDOW_DRAWLIST;

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasBottomRight = ImVec2(
        canvasTopLeft.x + canvasSize.x,
        canvasTopLeft.y + canvasSize.y
    );

    ImVec2 imageSize = ImVec2(350, 350);

    ImVec2 imageTopLeft = ImVec2(
        canvasTopLeft.x - 20,

        canvasTopLeft.y + ((canvasSize.y - imageSize.y) / 2) -
        ((cosf(ImGui::GetTime()) + 1) * 15) + 8
    );
    ImVec2 imageBottomRight = ImVec2(
        imageTopLeft.x + imageSize.x,
        imageTopLeft.y + imageSize.y
    );

    drawList->AddImage((void*)(intptr_t)this->image.texture, imageTopLeft, imageBottomRight);

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

    ImGui::PushFont(appState.fontGiant);

    {
        float addX{ 0.f };

        const char* p = mainStrings[0];
        float distance{ 0 };
        while (*p) {
            char buffer[2];
            buffer[0] = *p;
            buffer[1] = '\0';

            float graphResult = spinGraph(ImGui::GetTime() - (distance * .25f));

            ImVec2 position(
                textPosition.x + addX +
                ((cosf((ImGui::GetTime() + (distance * 2)) * 7) * 6) * graphResult),

                textPosition.y +
                ((sinf((ImGui::GetTime() + (distance * 2)) * 7) * 6) * graphResult)
            );

            drawList->AddText(
                position,
                IM_COL32_WHITE,
                buffer
            );

            addX += ImGui::CalcTextSize(buffer).x;
        
            ++p;
            ++distance;
        }
    }

    textPosition.y += ImGui::GetTextLineHeight() + 5.f;
    ImGui::PopFont();

    drawList->AddText(textPosition, IM_COL32_WHITE, mainStrings[1]);

    if (ImGui::Button(mainStrings[2], { 30.f, 30.f })) {
        std::stringstream cmdStream;

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
                IM_COL32_WHITE, creditStrings[i].string
            );

        additiveLineHeight += ImGui::GetTextLineHeight() + 2.f;

        totalLineWidth = std::max(totalLineWidth, ImGui::CalcTextSize(creditStrings[i].string).x);
    }

    ImGui::End();
}