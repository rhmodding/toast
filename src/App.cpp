#include "App.hpp"

#include "font/CustomFont.h"
#include "font/SegoeUI.h"
#include "font/FontAwesome.h"

#include <iostream>
#include <vector>
#include <sstream>

#include "archive/U8.hpp"
#include "texture/TPL.hpp"

#include <imgui_internal.h>

#define GET_IMGUI_IO ImGuiIO& io = ImGui::GetIO();

GLuint LoadTPLTextureIntoGLTexture(TPL::TPLTexture tplTexture) {
    GLuint imageTexture;
    
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);

    GLint minFilter;
    GLint magFilter;

    switch (tplTexture.magFilter) {
        case TPL::TPL_TEX_FILTER_NEAR:
            magFilter = GL_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_LINEAR:
            magFilter = GL_LINEAR;
            break;
        default:
            magFilter = GL_LINEAR;
            break;
    }

    switch (tplTexture.minFilter) {
        case TPL::TPL_TEX_FILTER_NEAR:
            minFilter = GL_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_LINEAR:
            minFilter = GL_LINEAR;
            break;
        case TPL::TPL_TEX_FILTER_NEAR_MIP_NEAR:
            minFilter = GL_NEAREST_MIPMAP_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_LIN_MIP_NEAR:
            minFilter = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_NEAR_MIP_LIN:
            minFilter = GL_NEAREST_MIPMAP_LINEAR;
            break;
        case TPL::TPL_TEX_FILTER_LIN_MIP_LIN:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            break;
        default:
            minFilter = GL_LINEAR;
            break;
    }

    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        tplTexture.wrapS == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
    );
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        tplTexture.wrapT == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tplTexture.width, tplTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return imageTexture;
}

App::App() {
    glfwSetErrorCallback([](int error_code, const char* description) {
        std::cerr << "GLFW Error (" << error_code << "): " << description << '\n';
    });
    assert(glfwInit());
    
    #if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glslVersion = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    #elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char* glslVersion = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    #else
        // GL 3.0 + GLSL 130
        const char* glslVersion = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    #endif

    this->window = glfwCreateWindow(1280, 720, WINDOW_TITLE, nullptr, nullptr);
    assert(this->window);

    glfwMakeContextCurrent(this->window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    GET_IMGUI_IO;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    this->UpdateTheme();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.f;
    }

    style.WindowPadding.x = 10;
    style.WindowPadding.y = 10;

    style.FrameRounding = 5;
    style.WindowRounding = 5;

    style.FramePadding.x = 5;
    style.FramePadding.y = 3;

    style.ItemSpacing.x = 9;
    style.ItemSpacing.y = 4;

    style.PopupRounding = 5;

    style.WindowTitleAlign.x = 0.5f;
    style.WindowTitleAlign.y = 0.5f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    this->SetupFonts();

    this->fileTabs[0].open = true;
    this->fileTabs[0].name = "File no. 1##FileTab0";

    // Init brcad

    auto archiveResult = U8::readYaz0U8Archive("E:\\Dolphin-x64\\Games\\tengoku-j\\content2\\cellanim\\interview\\ver0\\cellanim.szs");
    if (!archiveResult.has_value()) {
        std::cerr << '\n' << "Error loading archive!" << '\n';
        return;
    }

    U8::U8ArchiveObject archiveObject = archiveResult.value();

    auto cellAnimSearch = U8::findFile("./cellanim.tpl", archiveObject.structure);
    if (!cellAnimSearch.has_value()) {
        std::cerr << '\n' << "Error finding cellanim.tpl! Does it exist?" << '\n';
        return;
    }

    U8::File cellanimFile = cellAnimSearch.value();

    TPL::TPLObject cellanimTPL = TPL::TPLObject(cellanimFile.data.data(), cellanimFile.data.size());

    std::vector<const U8::File*> brcadFiles;
    for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
        if (file.name.size() >= 6 && file.name.substr(file.name.size() - 6) == ".brcad") {
            brcadFiles.push_back(&file);
        }
    }

    uint16_t brcadIndex{ 7 };

    {
        const U8::File* headerFile{ nullptr };
        std::string targetHeaderName =
            "rcad_" +
            brcadFiles.at(brcadIndex)->name.substr(0, brcadFiles.at(brcadIndex)->name.size() - 6) +
            "_labels.h";

        for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
            if (file.name == targetHeaderName) {
                headerFile = &file;
                break;
            }
        }

        if (!headerFile)
            return;

        std::istringstream stringStream(std::string(headerFile->data.begin(), headerFile->data.end()));
        std::string line;

        while (std::getline(stringStream, line)) {
            if (line.compare(0, 7, "#define") == 0) {
                std::istringstream lineStream(line);
                std::string define, key;
                uint16_t value;

                lineStream >> define >> key >> value;

                size_t commentPos = key.find("//");
                if (commentPos != std::string::npos) {
                    key = key.substr(0, commentPos);
                }

                this->animationNames.insert({ value, key });
            }
        }
    }

    this->cellanim = new RvlCellAnim::RvlCellAnimObject(brcadFiles.at(brcadIndex)->data.data(), brcadFiles.at(brcadIndex)->data.size());

    TPL::TPLTexture tplTexture = cellanimTPL.textures.at(this->cellanim->sheetIndex);

    this->cellanimSheet.width = tplTexture.width;
    this->cellanimSheet.height = tplTexture.height;
    this->cellanimSheet.texture = LoadTPLTextureIntoGLTexture(tplTexture);

    this->animatable = new Animatable(this->cellanim, &this->cellanimSheet);
    this->animatable->setAnimation(0);
    
    GET_APP_STATE;

    appState.playerState.setAnimatable(this->animatable);
    appState.playerState.updateSetFrameCount();
}

void App::Stop() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    Common::deleteIfNotNullptr(this->cellanim);
    Common::deleteIfNotNullptr(this->animatable);
}

void App::SetupFonts() {
    GET_IMGUI_IO;

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 18.0f, &fontConfig);

    {
        static const ImWchar icons_ranges[] = { ICON_MIN_IGFD, ICON_MAX_IGFD, 0 };

        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;

        io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_IGFD, 15.0f, &icons_config, icons_ranges);
    }

    {
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;

        io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_FA, 15.0f, &icons_config, icons_ranges);
    }
}

void App::Menubar() {
    GET_APP_STATE;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(WINDOW_TITLE)) {
            if (ImGui::MenuItem((char*)ICON_IGFD_RESET " About " WINDOW_TITLE, "", nullptr)) {
                appState.showAboutWindow = true;
                ImGui::SetWindowFocus("About " WINDOW_TITLE);
            }

            ImGui::Separator();

            if (ImGui::MenuItem((char*)ICON_IGFD_CANCEL " Quit", "Alt+F4", nullptr))
                this->quit = 0xFF;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMenuBar()) {
        ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll;

        const char* names[4] = { "Artichoke", "Beetroot", "Celery", "Daikon" };
        static bool opened[4] = { true, true, true, true }; // Persistent user state
        if (ImGui::BeginTabBar("MyTabBar", tabBarFlags)) {
            for (int n = 0; n < IM_ARRAYSIZE(fileTabs); n++)
                if (fileTabs[n].open && ImGui::BeginTabItem(fileTabs[n].name, &fileTabs[n].open, ImGuiTabItemFlags_None))
                    ImGui::EndTabItem();
            ImGui::EndTabBar();
        }

        ImGui::EndMenuBar();
    }
}

void App::Animations() {
    GET_APP_STATE;

    ImGui::Begin("Animations", nullptr);

    static uint16_t selected{ 0 };

    ImGui::BeginChild("AnimationList", ImVec2(0, 0), ImGuiChildFlags_Border);
    {
        for (uint16_t n = 0; n < this->animatable->cellanim->animations.size(); n++) {
            char buf[128];

            auto it = this->animationNames.find(n);
            sprintf_s(buf, 128, "%d. %s", n, it != this->animationNames.end() ? it->second.c_str() : "(no macro defined)");

            if (ImGui::Selectable(buf, selected == n)) {
                selected = n;

                this->animatable->setAnimation(selected);

                appState.playerState.currentFrame = 0;
                appState.playerState.updateSetFrameCount();
                appState.playerState.updateCurrentFrame();
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void App::Canvas() {
    static bool drawPartOrigin{ false };
    static ImVec4 partOriginDrawColor{ 1, 1, 1, 1 };
    static float partOriginDrawRadius{ 5.f };

    static bool allowOpacity{ true };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    static enum GridType : uint8_t {
        GridType_None,
        GridType_Dark,
        GridType_Light,
        
        GridType_Custom
    } gridType{ AppState::getInstance().darkTheme ? GridType_Dark : GridType_Light };
    static ImVec4 customGridColor{ 1.f, 1.f, 1.f, 1.f };

    static ImVector<ImVec2> points;
    static ImVec2 canvasUserOffset(0.0f, 0.0f); // User view pan
    static bool opt_enable_context_menu{ true };
    static bool adding_line{ false };

    static float zoomState{ 0.f };

    ImVec2 canvasTopLeft = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
    ImVec2 canvasBottomRight = ImVec2(canvasTopLeft.x + canvasSize.x, canvasTopLeft.y + canvasSize.y);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Grid")) {
            if (ImGui::MenuItem("None", nullptr, gridType == GridType_None))
                gridType = GridType_None;

            if (ImGui::MenuItem("Dark", nullptr, gridType == GridType_Dark))
                gridType = GridType_Dark;

            if (ImGui::MenuItem("Light", nullptr, gridType == GridType_Light))
                gridType = GridType_Light;

            if (ImGui::BeginMenu("Custom")) {
                bool enabled = gridType == GridType_Custom;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    if (enabled)
                        gridType = GridType_Custom;
                    else
                        gridType = AppState::getInstance().darkTheme ? GridType_Dark : GridType_Light;
                };

                ImGui::SeparatorText("Color Picker");

                ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&customGridColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset", nullptr, false)) {
                canvasUserOffset = { 0, 0 };
                zoomState = 0;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Rendering")) {
            if (ImGui::BeginMenu("Draw Part Origin")) {
                ImGui::Checkbox("Enabled", &drawPartOrigin);

                ImGui::SeparatorText("Options");

                ImGui::DragFloat("Radius", &partOriginDrawRadius, 0.1f);

                ImGui::SeparatorText("Color Picker");

                ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&partOriginDrawColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Enable part transparency", nullptr, allowOpacity))
                allowOpacity = !allowOpacity;

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    uint32_t backgroundColor;
    switch (gridType) {
        case GridType_None:
            backgroundColor = IM_COL32(0, 0, 0, 0);
            break;
        
        case GridType_Dark:
            backgroundColor = IM_COL32(50, 50, 50, 255);
            break;

        case GridType_Light:
            backgroundColor = IM_COL32(255, 255, 255, 255);
            break;

        case GridType_Custom:
            backgroundColor = IM_COL32(
                customGridColor.x * 255,
                customGridColor.y * 255,
                customGridColor.z * 255,
                customGridColor.w * 255
            );
            break;
    
        default:
            break;
    }

    GET_IMGUI_IO;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions
    ImGui::InvisibleButton("CanvasInteractions", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle
    );

    const bool interactionHovered = ImGui::IsItemHovered();
    const bool interactionActive = ImGui::IsItemActive(); // Held

    const ImVec2 origin(
        canvasTopLeft.x + canvasUserOffset.x + static_cast<int>(canvasSize.x / 2),
        canvasTopLeft.y + canvasUserOffset.y + static_cast<int>(canvasSize.y / 2)
    ); // Lock scrolled origin
    const ImVec2 relativeMousePos(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

    // Add first and second point
    if (interactionHovered && !adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    {
        points.push_back(relativeMousePos);
        points.push_back(relativeMousePos);
        adding_line = true;
    }
    if (adding_line)
    {
        points.back() = relativeMousePos;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            adding_line = false;
    }

    // Pan (we use a zero mouse threshold when there's no context menu)
    // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
    const float mousePanThreshold = opt_enable_context_menu ? -1.0f : 0.0f;

    bool panningCanvas = interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold)
    );

    if (panningCanvas) {
        canvasUserOffset.x += io.MouseDelta.x;
        canvasUserOffset.y += io.MouseDelta.y;
    }

    if (interactionHovered) {
        GET_IMGUI_IO;

        if (io.KeyShift) 
            zoomState += io.MouseWheel * CANVAS_ZOOM_SPEED_FAST;
        else
            zoomState += io.MouseWheel * CANVAS_ZOOM_SPEED;

        if (zoomState < -0.90f)
            zoomState = -0.90f;

        if (zoomState > 9.f)
            zoomState = 9.f;
    }

    // Context menu (under default mouse threshold)
    ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    if (opt_enable_context_menu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
        ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
    if (ImGui::BeginPopup("context"))
    {
        if (adding_line)
            points.resize(points.size() - 2);

        adding_line = false;
        if (ImGui::MenuItem("Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 2); }
        if (ImGui::MenuItem("Remove all", NULL, false, points.Size > 0)) { points.clear(); }
        ImGui::EndPopup();
    }

    // Draw grid + all lines in the canvas
    drawList->PushClipRect(canvasTopLeft, canvasBottomRight, true);

    if (gridType != GridType_None) {
        float GRID_STEP = 64.0f * (zoomState + 1);

        for (float x = fmodf(canvasUserOffset.x + static_cast<int>(canvasSize.x / 2), GRID_STEP); x < canvasSize.x; x += GRID_STEP)
            drawList->AddLine(ImVec2(canvasTopLeft.x + x, canvasTopLeft.y), ImVec2(canvasTopLeft.x + x, canvasBottomRight.y), IM_COL32(200, 200, 200, 40));
        
        for (float y = fmodf(canvasUserOffset.y + static_cast<int>(canvasSize.y / 2), GRID_STEP); y < canvasSize.y; y += GRID_STEP)
            drawList->AddLine(ImVec2(canvasTopLeft.x, canvasTopLeft.y + y), ImVec2(canvasBottomRight.x, canvasTopLeft.y + y), IM_COL32(200, 200, 200, 40));
    }

    for (int n = 0; n < points.Size; n += 2)
        drawList->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);

    bool animatableDoesDraw = this->animatable->getDoesDraw(allowOpacity);

    if (animatableDoesDraw) {
        this->animatable->scaleX = zoomState + 1;
        this->animatable->scaleY = zoomState + 1;

        this->animatable->offset = origin;
        this->animatable->Draw(drawList, allowOpacity);
    }

    if (drawPartOrigin)
        this->animatable->DrawOrigins(drawList, partOriginDrawRadius, IM_COL32(
            partOriginDrawColor.x * 255,
            partOriginDrawColor.y * 255,
            partOriginDrawColor.z * 255,
            partOriginDrawColor.w * 255
        ));

    drawList->PopClipRect();

    ImU32 textColor;

    switch (gridType) {
        case GridType_None:
            textColor = AppState::getInstance().darkTheme ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
            break;
        
        case GridType_Dark:
            textColor = IM_COL32(255, 255, 255, 255);
            break;

        case GridType_Light:
            textColor = IM_COL32(0, 0, 0, 255);
            break;

        case GridType_Custom: {
            float y = 0.2126f * customGridColor.x + 0.7152f * customGridColor.y + 0.0722f * customGridColor.z;

            if (y > 0.5f)
                textColor = IM_COL32(0, 0, 0, 255);
            else
                textColor = IM_COL32(255, 255, 255, 255);
        } break;
    
        default:
            break;
    }

    float textDrawHeight = canvasTopLeft.y + 5.f;

    if ((canvasUserOffset.x != 0.f) || (canvasUserOffset.y != 0.f)) {
        char buffer[64] = { '\0' };
        sprintf_s((char*)&buffer, 64, "Offset: %f, %f", canvasUserOffset.x, canvasUserOffset.y);

        drawList->AddText(ImVec2(canvasTopLeft.x + 10, textDrawHeight), textColor, buffer);

        textDrawHeight += 18.f;
    }

    if (zoomState != 0.f) {
        char buffer[64] = { '\0' };
        sprintf_s((char*)&buffer, 64, "Zoom: %u%% (hold [Shift] to zoom faster)", static_cast<uint16_t>((zoomState + 1) * 100));

        drawList->AddText(
            ImVec2(
                canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, buffer
        );

        textDrawHeight += 18.f;
    }

    if (!animatableDoesDraw) {
        const char* text = "Nothing to draw on this frame";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        drawList->AddText(
            ImVec2(
                canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, text
        );

        textDrawHeight += 18.f;
    }

    ImGui::End();
}

void App::Inspector() {
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        ImGui::Text("Placeholder");

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void App::Update() {
    GET_APP_STATE;
    static ImGuiIO& io{ ImGui::GetIO() };

    glfwPollEvents();

    // Start frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    this->BeginMainWindow(io);

    this->Menubar();

    if (appState.enableDemoWindow)
        ImGui::ShowDemoWindow(&appState.enableDemoWindow);

    static bool show_another_window{ false };

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &appState.enableDemoWindow);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&appState.windowClearColor); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    appState.playerState.Update();
    this->Canvas();

    this->Animations();

    this->Inspector();

    // TODO: implement timeline auto-scroll
    // static bool autoScrollTimeline{ false };

    {
        static float f = 0.0f;
        static int counter = 0;

        static uint16_t frameRate{60};

        static uint16_t u16One{ 1 };

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::Begin("Timeline");
        ImGui::PopStyleVar();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));

        ImGui::BeginChild("TimelineToolbar", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);

        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 0.5f));
        
        ImGui::Dummy(ImVec2(2, 0));
        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(15, 0));
        
        if (ImGui::BeginTable("TimelineToolbarTable", 2, ImGuiTableFlags_BordersInnerV)) {
            for (uint16_t row = 0; row < 1; row++) {
                ImGui::TableNextRow();
                for (uint16_t column = 0; column < 2; column++) {
                    ImGui::TableSetColumnIndex(column);

                    switch (column) {
                        case 0: {
                            char playPauseButtonLabel[32] = { '\0' };
                            const char* playPauseIcon = appState.playerState.playing ? (char*)ICON_FA_PAUSE : (char*)ICON_FA_PLAY;

                            strcpy_s(playPauseButtonLabel, 32, playPauseIcon);
                            strcat_s(playPauseButtonLabel, 32, "##playPauseButton");

                            if (ImGui::Button(playPauseButtonLabel, ImVec2(32, 32))) {
                                if (
                                    (appState.playerState.currentFrame == appState.playerState.frameCount - 1) &&
                                    (appState.playerState.holdFramesLeft == 0)
                                ) {
                                    appState.playerState.currentFrame = 0;
                                    appState.playerState.updateCurrentFrame();
                                }

                                appState.playerState.ResetTimer();
                                appState.playerState.ToggleAnimating(!appState.playerState.playing);
                            } 
                            ImGui::SameLine();

                            ImGui::Dummy(ImVec2(2, 0));
                            ImGui::SameLine();

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_BACKWARD_FAST "##firstFrameButton", ImVec2(32-6, 32-6))) {
                                appState.playerState.currentFrame = 0;
                                appState.playerState.updateCurrentFrame();
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Go to first key");

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_BACKWARD_STEP "##backFrameButton", ImVec2(32-6, 32-6))) {
                                if (appState.playerState.currentFrame >= 1) {
                                    appState.playerState.currentFrame--;
                                    appState.playerState.updateCurrentFrame();
                                }
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Step back a key");
                            
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_STOP "##stopButton", ImVec2(32-6, 32-6))) {
                                appState.playerState.playing = false;
                                appState.playerState.currentFrame = 0;
                                appState.playerState.updateCurrentFrame();
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Stop playback and go to first key");

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_FORWARD_STEP "##forwardFrameButton", ImVec2(32-6, 32-6))) {
                                if (appState.playerState.currentFrame < appState.playerState.frameCount - 1) {
                                    appState.playerState.currentFrame++;
                                    appState.playerState.updateCurrentFrame();
                                }
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Step forward a key");
                            
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                            if (ImGui::Button((char*)ICON_FA_FORWARD_FAST "##lastFrameButton", ImVec2(32-6, 32-6))) {
                                appState.playerState.currentFrame =
                                    appState.playerState.frameCount -
                                    (appState.playerState.frameCount > 0 ? 1 : 0);

                                appState.playerState.updateCurrentFrame();
                            } ImGui::SameLine();

                            ImGui::SetItemTooltip("Go to last key");

                            ImGui::Dummy(ImVec2(2, 0));
                            ImGui::SameLine();

                            {
                                bool popColor{ false }; // Use local boolean since state can be mutated by button
                                if (appState.playerState.loopEnabled) {
                                    popColor = true;
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                                }

                                if (ImGui::Button((char*)ICON_FA_ARROW_ROTATE_RIGHT, ImVec2(32, 32)))
                                    appState.playerState.loopEnabled = !appState.playerState.loopEnabled;

                                ImGui::SetItemTooltip("Toggle looping");

                                if (popColor)
                                    ImGui::PopStyleColor();
                            }
                        } break;

                        case 1: {
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);

                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                            if (ImGui::InputScalar("Frame", ImGuiDataType_U16, &appState.playerState.currentFrame, nullptr, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue)) {
                                appState.playerState.updateCurrentFrame();
                            }

                            ImGui::SameLine();

                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::InputScalar("Hold Frames Left", ImGuiDataType_U16, &appState.playerState.holdFramesLeft, nullptr, nullptr, "%u", ImGuiInputTextFlags_ReadOnly);
                            ImGui::PopItemFlag();

                            ImGui::SameLine();

                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("65536").x + 15);
                            ImGui::InputScalar("FPS", ImGuiDataType_U16, &appState.playerState.frameRate, nullptr, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue);
                        } break;

                        case 2: {
                            /*
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);

                            ImGui::Checkbox("Auto-scroll timeline", &autoScrollTimeline);
                            */
                        } break;

                        default: {

                        } break;
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        ImGui::Dummy(ImVec2(0, 0.5f));

        ImGui::EndChild();

        ImGui::BeginChild("TimelineKeys", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar);

        const ImVec2 buttonDimensions(22.f, 30.f);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(15, 0));
        
        if (ImGui::BeginTable("TimelineFrameTable", 2,
                                                        ImGuiTableFlags_RowBg |
                                                        ImGuiTableFlags_BordersInnerV |
                                                        ImGuiTableFlags_ScrollX
        )) {
            ImGui::TableNextRow();
            {
                ImGui::TableSetColumnIndex(0);
                ImGui::Dummy(ImVec2(5, 0));
                ImGui::SameLine();
                ImGui::TextUnformatted("Keys");
                ImGui::TableSetColumnIndex(1);

                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);

                    for (uint16_t i = 0; i < appState.playerState.frameCount; i++) {
                        ImGui::PushID(i);

                        bool popColor{ false };
                        if (appState.playerState.currentFrame == i) {
                            popColor = true;
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        }

                        char buffer[16];
                        sprintf_s(buffer, 16, "%u##KeyButton", i+1);

                        if (ImGui::Button(buffer, buttonDimensions)) {
                            appState.playerState.currentFrame = i;
                            appState.playerState.updateCurrentFrame();
                        }

                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text((char*)ICON_FA_KEY "  Key no. %u", i+1);

                            ImGui::Separator();

                            RvlCellAnim::AnimationKey* key = &this->animatable->getCurrentAnimation()->keys.at(i);

                            ImGui::BulletText("Arrangement Index: %u", key->arrangementIndex);
                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::BulletText("Held for: %u frame(s)", key->holdFrames);
                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::BulletText("Scale X: %f", key->scaleX);
                            ImGui::BulletText("Scale Y: %f", key->scaleY);
                            ImGui::BulletText("Angle: %f", key->angle);
                            ImGui::Dummy(ImVec2(0, 10));
                            ImGui::BulletText("Opacity: %u/255", key->opacity);

                            ImGui::EndTooltip();
                        }

                        if (popColor)
                            ImGui::PopStyleColor();

                        // Hold frame dummy

                        uint16_t holdFrames = this->animatable->getCurrentAnimation()->keys.at(i).holdFrames;
                        if (holdFrames > 1) {
                            ImGui::SameLine();
                            ImGui::Dummy(ImVec2(static_cast<float>(10 * holdFrames), buttonDimensions.y));
                        }

                        ImGui::SameLine();

                        ImGui::PopID();
                    }

                    ImGui::PopStyleVar(3);
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableSetColumnIndex(0);
                ImGui::Dummy(ImVec2(5, 0));
                ImGui::SameLine();
                ImGui::TextUnformatted("Hold Frames");
                ImGui::TableSetColumnIndex(1);

                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);

                    for (uint16_t i = 0; i < appState.playerState.frameCount; i++) {
                        ImGui::PushID(i);

                        // Key button dummy
                        ImGui::Dummy(buttonDimensions);

                        uint16_t holdFrames = this->animatable->getCurrentAnimation()->keys.at(i).holdFrames;
                        if (holdFrames > 1) {
                            ImGui::SameLine();

                            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(
                                appState.playerState.currentFrame == i ?
                                    ((holdFrames - appState.playerState.holdFramesLeft) / (float)holdFrames) :
                                appState.playerState.currentFrame > i ?
                                    1.0f : 0.0f,
                                0.5f
                            ));

                            ImGui::BeginDisabled();
                            ImGui::Button((char*)ICON_FA_HOURGLASS "", { static_cast<float>(10 * holdFrames), 30.f });
                            ImGui::EndDisabled();

                            ImGui::PopStyleVar();
                        }

                        ImGui::SameLine();

                        ImGui::PopID();
                    }

                    ImGui::PopStyleVar(3);
                }
            }
        
            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        /*
        if (autoScrollTimeline && appState.playerState.playing) {
            ImGui::SetScrollX(ImGui::GetScrollMaxX() * appState.playerState.getAnimationProgression());
        }
        */

        ImGui::EndChild();

        ImGui::End();
    }

    // End main window
    ImGui::End();

    // Render
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(
        appState.windowClearColor.x * appState.windowClearColor.w,
        appState.windowClearColor.y * appState.windowClearColor.w,
        appState.windowClearColor.z * appState.windowClearColor.w,
        appState.windowClearColor.w
    );
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        glfwMakeContextCurrent(backupCurrentContext);
    }

    glfwSwapBuffers(window);
}