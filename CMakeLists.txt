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

    src/main.cpp
    src/App.cpp

    src/anim/Animatable.cpp
    src/anim/RevCellAnim.cpp

    src/archive/U8.cpp

    src/compression/Yaz0.cpp

    src/texture/ImageConvert.cpp
    src/texture/TPL.cpp

    src/common.cpp

    src/stb/stb_image.h
)

set(HEADERS
    ext/imgui/imconfig.h
    ext/imgui/imgui.h
    ext/imgui/imgui_internal.h
    ext/imgui/backends/imgui_impl_glfw.h
    ext/imgui/backends/imgui_impl_opengl3.h
    ext/imgui/backends/imgui_impl_opengl3_loader.h

    src/App.hpp

    src/anim/Animatable.hpp
    src/anim/RevCellAnim.hpp

    src/archive/U8.hpp

    src/compression/Yaz0.hpp

    src/texture/ImageConvert.hpp
    src/texture/TPL.hpp

    src/common.hpp
)

IF (APPLE)
    add_executable (toast MACOSX_BUNDLE ${SOURCES} ${HEADERS})
ELSE()
    add_executable (toast ${SOURCES} ${HEADERS})
ENDIF()

# Include directories
target_include_directories(toast PRIVATE
    ext/imgui/
    ext/imgui/backends
    libs/glfw/include
)

# Link directories
IF (APPLE) # macOS
    target_link_directories(toast PRIVATE
        /usr/local/Cellar/glfw/3.3.8/lib
    )
ELSE() # Generic
    target_link_directories(toast PRIVATE
        libs/glfw/lib-vc2010-64
    )
ENDIF()

# Link libraries
IF (WIN32) # Windows
    target_link_libraries(toast PRIVATE
        opengl32.lib
        glfw3.lib
    )
ELSEIF(APPLE) # macOS
    target_link_libraries(toast PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        glfw
    )
ELSE() # Generic
    target_link_libraries(toast PRIVATE
        GL
        glfw
    )
ENDIF()

# Link directory for Linux
IF (UNIX AND NOT APPLE) # Linux
    link_directories(toast PRIVATE
        /usr/lib/x86_64-linux-gnu/
    )
ENDIF()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET toast PROPERTY CXX_STANDARD 20)
endif()
