// toast
// conhlee
// 2024

#include "App.hpp"

#include "ConsoleSplash.hpp"

int main(int argc, char** argv) {
    // TODO: open szs from arguments
    (void)argc; (void)argv;

    std::cout << consoleSplash << "\n\n";

    App app;
    while (LIKELY(app.isRunning()))
        app.Update();
}
