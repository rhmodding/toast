// toast
// conhlee
// 2024

#include "App.hpp"

#include "ConsoleSplash.hpp"

int main(int argc, const char** argv) {
    std::cout << consoleSplash << "\n\n";

    App app (argc, argv);
    while (LIKELY(app.isRunning()))
        app.Update();
}
