// toast
// conhlee
// 2024

#include "App.hpp"

#include "AppState.hpp"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// Main code
int main(int argc, char** argv) {
    new App;

    extern App* gAppPtr;

    while (!gAppPtr->quit)
        gAppPtr->Update();

    gAppPtr->Stop();

    delete gAppPtr;

    return 0;
}
