# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")

project(toast)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

IF (MSVC)
    message(FATAL_ERROR "Compiling with MSVC is not supported. Please use GCC or Clang instead")
ENDIF()

set(SOURCES
    src/BuildDate.cpp

    ext/imgui/imgui.cpp
    ext/imgui/imgui_demo.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_glfw.cpp
    ext/imgui/backends/imgui_impl_opengl3.cpp

    ext/rg-etc1/rg_etc1.cpp

    ext/tinyfiledialogs/tinyfiledialogs.c

    src/App/Actions.cpp
    src/App/PopupHandler.cpp
    src/App/Shortcuts.cpp

    src/App/popups/SheetRepackFailed.cpp
    src/App/popups/EditAnimationName.cpp
    src/App/popups/EditPartName.cpp
    src/App/popups/SpritesheetManager.cpp
    src/App/popups/SwapAnimation.cpp
    src/App/popups/WaitForModifiedTexture.cpp
    src/App/popups/ModifiedTextureSize.cpp
    src/App/popups/MInterpolateKeys.cpp
    src/App/popups/MOptimizeGlobal.cpp
    src/App/popups/MPadRegion.cpp
    src/App/popups/MTransformAnimation.cpp
    src/App/popups/MTransformArrangement.cpp
    src/App/popups/MTransformCellanim.cpp

    src/archive/Archive.cpp
    src/archive/DARCH.cpp
    src/archive/SARC.cpp

    src/BIN/fontdata/FontAwesome.cpp
    src/BIN/fontdata/NotoSans.cpp
    src/BIN/fontdata/NotoSansJP.cpp

    src/BIN/image/toastIcon_title.png.cpp
    src/BIN/image/toastIcon.png.cpp

    src/cellanim/CellAnim.cpp
    src/cellanim/CellAnimRenderer.cpp

    src/compression/Yaz0/Compress.cpp
    src/compression/Yaz0/Window.cpp

    src/compression/NZlib.cpp
    src/compression/Yaz0.cpp

    src/manager/AsyncTaskManager.cpp
    src/manager/ConfigManager.cpp
    src/manager/MainThreadTaskManager.cpp
    src/manager/PlayerManager.cpp
    src/manager/PromptPopupManager.cpp
    src/manager/SessionManager.cpp
    src/manager/ThemeManager.cpp

    src/stb/stb_dxt_impl.cpp
    src/stb/stb_image_impl.cpp
    src/stb/stb_image_resize2_impl.cpp
    src/stb/stb_image_write_impl.cpp
    src/stb/stb_rect_pack_impl.cpp

    src/task/AsyncTask.cpp
    src/task/AsyncTaskExportSession.cpp
    src/task/AsyncTaskOptimizeCellanim.cpp
    src/task/AsyncTaskPushSession.cpp

    src/texture/APNGHack.cpp
    src/texture/CTPK.cpp
    src/texture/CtrImageConvert.cpp
    src/texture/RvlImageConvert.cpp
    src/texture/RvlPalette.cpp
    src/texture/Texture.cpp
    src/texture/TextureEx.cpp
    src/texture/TPL.cpp

    src/util/ArrangePartMatchUtil.cpp
    src/util/BezierUtil.cpp
    src/util/CxxDemangleUtil.cpp
    src/util/FileUtil.cpp
    src/util/MyPathUtil.cpp
    src/util/ShiftJISUtil.cpp
    src/util/SpritesheetFixUtil.cpp
    src/util/TweenAnimUtil.cpp
    src/util/UIUtil.cpp

    src/window/WindowInspector/LvlAnimation.cpp
    src/window/WindowInspector/LvlArrangement.cpp
    src/window/WindowInspector/LvlKey.cpp

    src/window/WindowAbout.cpp
    src/window/WindowCanvas.cpp
    src/window/WindowConfig.cpp
    src/window/WindowHybridList.cpp
    src/window/WindowImGuiDemo.cpp
    src/window/WindowInspector.cpp
    src/window/WindowRoot.cpp
    src/window/WindowSpritesheet.cpp
    src/window/WindowTimeline.cpp

    src/EditorDataPackage.cpp

    src/Logging.cpp

    src/ConsoleSplash.cpp

    src/main.cpp

    src/SelectionState.cpp

    src/Session.cpp

    src/TerminateHandler.cpp

    src/Toast.cpp
)

IF (APPLE)
    add_executable (toast MACOSX_BUNDLE ${SOURCES})
ELSE()
    add_executable (toast ${SOURCES})
ENDIF()

target_compile_definitions(toast PRIVATE
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS
    IMGUI_DISABLE_DEFAULT_FONT
    IMGUI_DEFINE_MATH_OPERATORS
)
IF (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(toast PRIVATE RG_ETC1_BUILD_DEBUG)
ENDIF()

IF (WIN32)
    target_compile_definitions(toast PRIVATE GLEW_STATIC)
ENDIF()

# Include directories
target_include_directories(toast PRIVATE
    ${CMAKE_SOURCE_DIR}/src

    ${CMAKE_SOURCE_DIR}/ext/imgui
    ${CMAKE_SOURCE_DIR}/ext/imgui/backends
    ${CMAKE_SOURCE_DIR}/ext/tinyfiledialogs
    ${CMAKE_SOURCE_DIR}/ext/rg-etc1
    ${CMAKE_SOURCE_DIR}/ext/cparse

    ${CMAKE_SOURCE_DIR}/libs/glfw/include
)

# Link directories
IF (APPLE) # macOS
    target_include_directories(toast PRIVATE
        libs/glfw3-apple/include
    )
    target_link_directories(toast PRIVATE
        libs/glfw3-apple/lib-universal
    )
ELSEIF (WIN32) # Windows
    target_include_directories(toast PRIVATE
        libs/glfw3-win64/include
        libs/glew-2.2.0-win/include
    )
    target_link_directories(toast PRIVATE
        libs/glfw3-win64/lib-mingw-w64
        libs/glew-2.2.0-win/lib/Release/x64
    )
ENDIF()

# Link directory for Linux
IF (UNIX AND NOT APPLE) # Linux
    target_link_directories(toast PRIVATE
        /usr/lib/x86_64-linux-gnu/
    )
ENDIF()

# Link libraries
IF (WIN32) # Windows
    target_link_libraries(toast PRIVATE
        glfw3
        glew32s
        opengl32

        comdlg32
        ole32

        -static
    )
ELSEIF (APPLE) # macOS
    target_link_libraries(toast PRIVATE
        glfw3
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework ApplicationServices"
        iconv
    )
ELSE() # Generic
    target_link_libraries(toast PRIVATE
        GL
        GLEW
        glfw

        dl
        pthread
        m
        X11
    )
ENDIF()

IF (APPLE)
    set_target_properties(toast PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
    )
ENDIF()

# disable building of zlib-ng tests
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "Disable zlib-ng tests" FORCE)
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "Disable zlib-ng specific tests" FORCE)

# these optimizations are not supported on some CPUs and cause crashes when
# initializing zlib-ng
set(WITH_AVX2 OFF CACHE BOOL "Disable zlib-ng AVX2 optimizations" FORCE)
set(WITH_AVX512 OFF CACHE BOOL "Disable zlib-ng AVX512 optimizations" FORCE)
set(WITH_AVX512VNNI OFF CACHE BOOL "Disable zlig-ng AVX512VNNI optimizations" FORCE)

add_subdirectory(ext/zlib-ng)

target_link_libraries(toast PRIVATE zlibstatic)

# JSON library
add_subdirectory(ext/nlohmann.json)

target_link_libraries(toast PRIVATE nlohmann_json::nlohmann_json)

# Logging library
add_subdirectory(ext/spdlog)

target_link_libraries(toast PRIVATE spdlog::spdlog)

# Flags
target_compile_options(toast PRIVATE -O3)

# Apple "deprecated" OpenGL but never removed it lol
IF (APPLE)
    target_compile_definitions(toast PRIVATE
        GL_SILENCE_DEPRECATION
    )
ENDIF()

set_property(TARGET toast PROPERTY CXX_STANDARD 20)
