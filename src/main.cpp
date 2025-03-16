// toast
// conhlee
// 2025

#include "Toast.hpp"

#include "ConsoleSplash.hpp"

#include <iostream>

#include <locale>

int main(int argc, const char** argv) {
    std::setlocale(LC_ALL, "C.UTF-8");

    std::cout << consoleSplash << "\n" << std::endl;

    Toast toast (argc, argv);
    while (LIKELY(toast.isRunning())) {
        toast.Update();
        toast.Draw();
    }
}
