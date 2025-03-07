// toast
// conhlee
// 2025

#include "Toast.hpp"

#include "ConsoleSplash.hpp"

int main(int argc, const char** argv) {
    std::cout << consoleSplash << "\n" << std::endl;

    Toast toast (argc, argv);
    while (LIKELY(toast.isRunning())) {
        toast.Update();
        toast.Draw();
    }
}
