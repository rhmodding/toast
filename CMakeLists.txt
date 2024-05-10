# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(toast)

set(SOURCES
    ext/imgui/imgui.cpp
    ext/imgui/imgui_demo.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_glfw.cpp
    ext/imgui/backends/imgui_impl_opengl3.cpp

    ext/ImGuiFileDialog/ImGuiFileDialog.cpp

    src/main.cpp
    src/App.cpp

    src/anim/Animatable.cpp
    src/anim/RvlCellAnim.cpp

    src/archive/U8.cpp

    src/compression/Yaz0.cpp

    src/texture/ImageConvert.cpp
    src/texture/TPL.cpp

    src/window/WindowTimeline.cpp
    src/window/WindowCanvas.cpp
    src/window/WindowHybridList.cpp
    src/window/WindowInspector.cpp
    src/window/WindowSpritesheet.cpp

    src/window/WindowConfig.cpp

    src/common.cpp

    src/AppState.cpp

    src/SessionManager.cpp

    src/stb/stb_image.h
    src/stb/stb_image_write.h
)

set(HEADERS
    ext/imgui/imconfig.h
    ext/imgui/imgui.h
    ext/imgui/imgui_internal.h
    ext/imgui/backends/imgui_impl_glfw.h
    ext/imgui/backends/imgui_impl_opengl3.h
    ext/imgui/backends/imgui_impl_opengl3_loader.h

    ext/ImGuiFileDialog/ImGuiFileDialog.h

    src/App.hpp

    src/anim/Animatable.hpp
    src/anim/RvlCellAnim.hpp

    src/archive/U8.hpp

    src/compression/Yaz0.hpp

    src/texture/ImageConvert.hpp
    src/texture/TPL.hpp

    src/window/BaseWindow.hpp

    src/window/WindowTimeline.hpp
    src/window/WindowCanvas.hpp
    src/window/WindowHybridList.hpp
    src/window/WindowInspector.hpp
    src/window/WindowSpritesheet.hpp

    src/window/WindowConfig.hpp

    src/AppState.hpp

    src/SessionManager.hpp

    src/ConfigManager.hpp

    src/common.hpp

    src/ConsoleSplash.hpp
)

# Disable MSVC unsafe function warnings (strcpy, sprintf, etc..)
IF (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF (APPLE)
    add_executable (toast MACOSX_BUNDLE ${SOURCES} ${HEADERS})
ELSE()
    add_executable (toast ${SOURCES} ${HEADERS})
ENDIF()

# Include directories
target_include_directories(toast PRIVATE
    ext/imgui/
    ext/imgui/backends
    ext/ImGuiFileDialog
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
    )
    IF (MSVC)
        target_link_directories(toast PRIVATE
            libs/glfw3-win64/lib-vc2022
        )
    ELSE()
        target_link_directories(toast PRIVATE
            libs/glfw3-win64/lib-mingw-w64
        )
    ENDIF()
ENDIF()

# Link libraries
IF (WIN32) # Windows
    target_link_libraries(toast PRIVATE
        glfw3
        opengl32

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
        glfw

        -static
    )
ENDIF()

# Link directory for Linux
IF (UNIX AND NOT APPLE) # Linux
    link_directories(toast PRIVATE
        /usr/lib/x86_64-linux-gnu/
    )
ENDIF()

# zeldamods zlib for Yaz0
option(ZLIB_ENABLE_TESTS "" OFF)

add_subdirectory(ext/zeldamods.zlib-ng)

target_link_libraries(toast PRIVATE zlib)

# JSON library
add_subdirectory(ext/nlohmann.json)

target_link_libraries(toast PRIVATE nlohmann_json::nlohmann_json)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET toast PROPERTY CXX_STANDARD 20)
endif()
