#include "App.hpp"

#include "font/SegoeUI.h"
#include "font/FontAwesome.h"

#include <iostream>
#include <vector>
#include <sstream>

#include "archive/U8.hpp"
#include "texture/TPL.hpp"

#include "imgui_internal.h"

App* gAppPtr{ nullptr };

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

    gAppPtr = this;
    glfwSetWindowCloseCallback(this->window, [](GLFWwindow* window) {
        std::cout << "Window close callback\n";

        extern App* gAppPtr;
        gAppPtr->quit = 0xFF;
    });

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

    this->windowCanvas = new WindowCanvas;
    this->windowCanvas->animatable = this->animatable;

    this->windowHybridList = new WindowHybridList;
    this->windowHybridList->animatable = this->animatable;
    this->windowHybridList->animationNames = &this->animationNames;

    this->windowInspector = new WindowInspector;
    this->windowInspector->animatable = this->animatable;
    this->windowInspector->animationNames = &this->animationNames;

    this->windowTimeline = new WindowTimeline;
    this->windowTimeline->animatable = this->animatable;

    this->windowSpritesheet = new WindowSpritesheet;
    this->windowSpritesheet->animatable = this->animatable;
    this->windowSpritesheet->sheet = &this->cellanimSheet;
}

void App::Stop() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    Common::deleteIfNotNullptr(this->cellanim);
    Common::deleteIfNotNullptr(this->animatable);

    Common::deleteIfNotNullptr(this->windowCanvas);
    Common::deleteIfNotNullptr(this->windowHybridList);
    Common::deleteIfNotNullptr(this->windowInspector);
    Common::deleteIfNotNullptr(this->windowTimeline);
    Common::deleteIfNotNullptr(this->windowSpritesheet);
}

void App::UpdateTheme() {
    GET_APP_STATE;

    if (appState.darkTheme) {
        ImGui::StyleColorsDark();
        appState.windowClearColor = ImVec4((24 / 255.f), (24 / 255.f), (24 / 255.f), 1.f);
    }
    else {
        ImGui::StyleColorsLight();
        appState.windowClearColor = ImVec4((248 / 255.f), (248 / 255.f), (248 / 255.f), 1.f);
    }  
}

void App::SetupFonts() {
    GET_IMGUI_IO;
    GET_APP_STATE;

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fontNormal = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 18.0f, &fontConfig);
    }

    {
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        ImFontConfig fontConfig;
        fontConfig.MergeMode = true;
        fontConfig.PixelSnapH = true;

        appState.fontIcon = io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_FA, 15.0f, &fontConfig, icons_ranges);
    }

    {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        appState.fontLarge = io.Fonts->AddFontFromMemoryTTF(SegoeUI_data, SegoeUI_length, 24.0f, &fontConfig);
    }
}

void App::Menubar() {
    GET_APP_STATE;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(WINDOW_TITLE)) {
            if (ImGui::MenuItem((char*)ICON_FA_SHARE_FROM_SQUARE " About " WINDOW_TITLE, "", nullptr)) {
                appState.showAboutWindow = true;
                ImGui::SetWindowFocus("About " WINDOW_TITLE);
            }

            ImGui::Separator();

            if (ImGui::MenuItem((char*)ICON_FA_DOOR_OPEN " Quit", "Alt+F4", nullptr))
                this->quit = 0xFF;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginMenuBar()) {
        ImGuiTabBarFlags tabBarFlags =
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton |
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll;

        if (ImGui::BeginTabBar("FileTabBar", tabBarFlags)) {
            for (int n = 0; n < ARRAY_LENGTH(fileTabs); n++)
                if (fileTabs[n].open && ImGui::BeginTabItem(fileTabs[n].name.c_str(), &fileTabs[n].open, ImGuiTabItemFlags_None))
                    ImGui::EndTabItem();
            ImGui::EndTabBar();
        }

        ImGui::EndMenuBar();
    }
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

    appState.playerState.Update();

    // Windows
    this->windowCanvas->Update();
    this->windowHybridList->Update();
    this->windowInspector->Update();
    this->windowTimeline->Update();
    this->windowSpritesheet->Update();

    // TODO: Implement timeline auto-scroll
    // static bool autoScrollTimeline{ false };

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