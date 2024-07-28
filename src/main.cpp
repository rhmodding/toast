// toast
// conhlee
// 2024

#include "App.hpp"

#include "ConsoleSplash.hpp"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

int main(int argc, char** argv) {
    std::cout << consoleSplash << "\n\n";

    {
        App app;
        while (LIKELY(app.isRunning()))
            app.Update();
    }
}
