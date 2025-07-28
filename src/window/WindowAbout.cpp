#include "WindowAbout.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>

#include <thread>

#include "font/FontAwesome.h"

#include "BIN/image/toastIcon_title.png.h"

#include "manager/ThemeManager.hpp"

#include "BuildDate.hpp"

enum LineCommand {
    LC_EMPTY_LINE,
    LC_EMPTY_LINE_QUART,
    LC_STRING_LINE,
    LC_SEPARATOR,
    LC_NEXT_ROW,
    LC_NEXT_COLUMN,
};
struct Line {
    LineCommand command;
    const char* string { nullptr };
};

static const char* headerTitle = "toast";
static const char* headerDesc  =
    "the ultimate cellanim modding tool for\n"
    "Rhythm Heaven Fever / Beat the Beat: Rhythm Paradise";

static const Line aboutLines[] = {
    { LC_SEPARATOR },

    { LC_STRING_LINE, (const char*)ICON_FA_WRENCH "  Development" },
    { LC_EMPTY_LINE_QUART },
    
    { LC_STRING_LINE, "conhlee" },

    // Padding
    { LC_STRING_LINE, "                                             " },

    { LC_STRING_LINE, (const char*)ICON_FA_MAGNIFYING_GLASS "  Debug" },
    { LC_EMPTY_LINE_QUART },
    
    { LC_STRING_LINE, "Kevbaum" },
    { LC_STRING_LINE, "Kievit" },
    { LC_STRING_LINE, "HaiKaede" },
    { LC_STRING_LINE, "BobTheNerd10" },
    { LC_STRING_LINE, "Deni_iguess" },

    { LC_NEXT_ROW },

    { LC_STRING_LINE, (const char*)ICON_FA_FILE_CODE "  Open-source software used / referenced" },
    { LC_EMPTY_LINE_QUART },

    { LC_STRING_LINE, "Dear ImGui (ocornut/imgui) [MIT]" },
    { LC_STRING_LINE, "GLFW [Zlib/libpng]" },
    { LC_STRING_LINE, "Various STB libraries (nothings/stb) [public domain]" },
    { LC_STRING_LINE, "zlib-ng [Zlib]" },
    { LC_STRING_LINE, "Orthrus (NWPlayer123/Orthrus) [MPL-2.0]" },
    { LC_STRING_LINE, "rg_etc1 (by Richard Geldreich) [Zlib]" },
    { LC_STRING_LINE, "tinyfiledialogs (by Guillaume Vareille) [Zlib]" },
    { LC_STRING_LINE, "json (nlohmann/json) [MIT]" },

    { LC_NEXT_COLUMN },

    { LC_STRING_LINE, (const char*)ICON_FA_STAR "  Special thanks" },
    { LC_EMPTY_LINE_QUART },
    { LC_STRING_LINE, "Chrislo (a.k.a. chrislo27)" },
    { LC_STRING_LINE, "patataofcourse" },
    { LC_STRING_LINE, "TheAlternateDoctor" },
    { LC_STRING_LINE, "EstexNT" },
    { LC_STRING_LINE, ".. and the rest of the RHModding community!" },
};

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

static char sBuildString[64];

constexpr ImVec2 windowSize (880.f, 520.f);

static void drawLines(const Line* lines, unsigned lineCount, ImVec2& position, ImGuiWindow* window) {
    float additiveLineHeight { 0.f };
    float totalLineWidth { 0.f };
    float additiveLineX { 0.f };

    for (unsigned i = 0; i < lineCount; i++) {
        const Line* line = lines + i;

        switch (line->command) {
        case LC_STRING_LINE:
            window->DrawList->AddText(
                { position.x + additiveLineX, position.y + additiveLineHeight },
                ImGui::GetColorU32(ImGuiCol_Text), line->string
            );
            break;

        case LC_SEPARATOR:
            position.y += ImGui::GetTextLineHeight();

            window->DrawList->AddRectFilled(
                { // Min
                    position.x, position.y + additiveLineHeight -
                    ( ImGui::GetTextLineHeight() / 2.f )
                },
                { // Max
                    window->WorkRect.Max.x - 25.f, position.y + additiveLineHeight -
                    ( ImGui::GetTextLineHeight() / 2.f ) + 1.f
                },
                ImGui::GetColorU32(ImGuiCol_Separator)
            );
            continue;

        case LC_NEXT_ROW:
            additiveLineX += totalLineWidth + 25.f;
            totalLineWidth = 0.f;

            additiveLineHeight = 0.f;
            continue;
        case LC_NEXT_COLUMN:
            position.y += additiveLineHeight + ImGui::GetTextLineHeight() + 6.f;
            additiveLineHeight = 0.f;

            totalLineWidth = 0.f;

            additiveLineX = 0.f;
            continue;

        case LC_EMPTY_LINE_QUART:
            additiveLineHeight += (ImGui::GetTextLineHeight() / 4.f) + 2.f;
            continue;

        case LC_EMPTY_LINE:
        default:
            break;
        }

        additiveLineHeight += ImGui::GetTextLineHeight() + 2.f;

        if (line->string != nullptr)
            totalLineWidth = std::max(totalLineWidth, ImGui::CalcTextSize(line->string).x);
    }
}

static void drawHeader(ImVec2& position, ImGuiWindow* window) {
    ThemeManager& themeManager = ThemeManager::getInstance();

    const float startX = position.x;

    // Set position for GitHub button.
    ImGui::SetCursorPos({
        window->WorkRect.Max.x - 25.f - 30.f - window->Pos.x,
        position.y + (30.f / 2.f) - window->Pos.y
    });

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 3.0f);

    // Title.
    window->DrawList->AddText(position, ImGui::GetColorU32(ImGuiCol_Text), headerTitle);

    position.x += ImGui::CalcTextSize(headerTitle).x + (ImGui::GetTextLineHeight() / 5.f);
    position.y += ImGui::GetTextLineHeight() / 1.8f;

    ImGui::PopFont();

    window->DrawList->AddText(position, ImGui::GetColorU32(ImGuiCol_TextDisabled), sBuildString);

    position.x = startX;
    position.y += ImGui::GetTextLineHeight() / .6f;

    // Description.
    window->DrawList->AddText(position, ImGui::GetColorU32(ImGuiCol_Text), headerDesc);

    // GitHub button.
    if (ImGui::Button(githubButton, { 30.f, 30.f })) {
        std::thread([]() {
            std::system(githubURLCmd);
        }).detach();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
        ImGui::SetTooltip("%s", githubTooltip);

    position.y += (ImGui::GetTextLineHeight() * 2.f) + 5.f;
}

WindowAbout::WindowAbout() {
    std::snprintf(sBuildString, sizeof(sBuildString), "built on: %s", gBuildDate);
}

void WindowAbout::Update() {
    if (!mOpen)
        return;

    if (mImage.getTextureId() == Texture::INVALID_TEXTURE_ID)
        mImage.LoadSTBMem(toastIcon_title_png, toastIcon_title_png_size);

    CENTER_NEXT_WINDOW();
    ImGui::SetNextWindowSize(windowSize);

    ImGui::Begin("About", &mOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* drawList = window->DrawList;

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
        ((std::cosf(ImGui::GetTime()) + 1) * 15.f) + 8.f
    };
    const ImVec2 imageBottomRight {
        imageTopLeft.x + imageSize.x,
        imageTopLeft.y + imageSize.y
    };

    drawList->AddImage(
        mImage.getImTextureId(),
        imageTopLeft, imageBottomRight
    );

    ImVec2 currentPosition (
        imageBottomRight.x + 10.f,
        canvasTopLeft.y + 5.f
    );

    drawHeader(currentPosition, window);
    drawLines(aboutLines, ARRAY_LENGTH(aboutLines), currentPosition, window);

    ImGui::End();
}
