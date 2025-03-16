# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(toast)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

IF (MSVC)
    message(FATAL_ERROR "Compiling with MSVC is not supported. Please use GCC or Clang instead")
ENDIF()

set(SOURCES
    ext/imgui/imgui.cpp
    ext/imgui/imgui_demo.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_glfw.cpp
    ext/imgui/backends/imgui_impl_opengl3.cpp

    ext/tinyfiledialogs/tinyfiledialogs.c

    ext/rg-etc1/rg_etc1.cpp

    src/stb/stb_dxt_impl.cpp
    src/stb/stb_image_write_impl.cpp
    src/stb/stb_image_impl.cpp
    src/stb/stb_rect_pack_impl.cpp

    src/main.cpp
    
    src/Toast.cpp

    src/Logging.cpp

    src/App/Popups.cpp

    src/anim/CellanimRenderer.cpp
    src/anim/CellAnim.cpp

    src/anim/CellanimHelpers.cpp

    src/archive/SARC.cpp
    src/archive/U8Archive.cpp

    src/compression/NZlib.cpp
    src/compression/Yaz0.cpp

    src/texture/Texture.cpp
    src/texture/TextureGroup.cpp

    src/texture/CtrImageConvert.cpp
    src/texture/RvlImageConvert.cpp

    src/texture/RvlPalette.cpp

    src/texture/CTPK.cpp
    src/texture/TPL.cpp

    src/task/AsyncTaskManager.cpp

    src/task/AsyncTask.cpp

    src/task/AsyncTaskExportSession.cpp
    src/task/AsyncTaskPushSession.cpp
    src/task/AsyncTaskOptimizeCellanim.cpp

    src/window/WindowTimeline.cpp
    src/window/WindowCanvas.cpp
    src/window/WindowHybridList.cpp
    src/window/WindowInspector.cpp
    src/window/WindowSpritesheet.cpp

    src/window/WindowInspector/WindowInspector_Animation.cpp
    src/window/WindowInspector/WindowInspector_Arrangement.cpp
    src/window/WindowInspector/WindowInspector_Key.cpp

    src/window/WindowConfig.cpp
    src/window/WindowAbout.cpp

    src/window/WindowImguiDemo.cpp

    src/EditorDataPackage.cpp

    src/SessionManager.cpp

    src/PlayerManager.cpp

    src/ConfigManager.cpp

    src/MainThreadTaskManager.cpp

    src/ThemeManager.cpp

    src/Files.cpp
)

IF (APPLE)
    add_executable (toast MACOSX_BUNDLE ${SOURCES} ${HEADERS})
ELSE()
    add_executable (toast ${SOURCES} ${HEADERS})
ENDIF()

target_compile_definitions(toast PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS IMGUI_DISABLE_DEFAULT_FONT)
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(toast PRIVATE RG_ETC1_BUILD_DEBUG)
ENDIF()

IF (WIN32)
target_compile_definitions(toast PRIVATE GLEW_STATIC)
ENDIF()

# Include directories
target_include_directories(toast PRIVATE
    ext/imgui
    ext/imgui/backends
    ext/tinyfiledialogs
    ext/rg-etc1

    libs/glfw/include
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
ELSEIF(APPLE) # macOS
    target_link_libraries(toast PRIVATE
        glfw3
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
ELSE() # Generic
    target_link_libraries(toast PRIVATE
        GL
        GLEW
        glfw

        dl
        pthread
        m
    )
ENDIF()

IF (APPLE)
    set_target_properties(toast PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
    )
ENDIF()

# zeldamods zlib for Yaz0 & NZlib
option(ZLIB_ENABLE_TESTS "" OFF)

add_subdirectory(ext/zeldamods.zlib-ng)

target_link_libraries(toast PRIVATE zlib)

# JSON library
add_subdirectory(ext/nlohmann.json)

target_link_libraries(toast PRIVATE nlohmann_json::nlohmann_json)

# Flags
target_compile_options(toast PRIVATE -O3 -Wno-address-of-packed-member -Wno-cast-align)

# Apple "deprecated" OpenGL but never removed it lol
IF (APPLE)
    target_compile_definitions(toast PRIVATE
        GL_SILENCE_DEPRECATION
    )
ENDIF()

set_property(TARGET toast PROPERTY CXX_STANDARD 20)
