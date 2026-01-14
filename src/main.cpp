// toast
// conhlee
// 2026

#include "Toast.hpp"

#include "ConsoleSplash.hpp"

#include "TerminateHandler.hpp"

#include <iostream>

#include <clocale>

int main(int argc, const char **argv) {
    std::setlocale(LC_ALL, "C.UTF-8");

    // Attach our custom terminate (exception) handler.
    TerminateHandler::init();

    std::cout << gConsoleSplash << '\n' << std::endl;

    Toast toast (argc, argv);
    while (LIKELY(toast.getRunning())) {
        toast.update();
        toast.draw();
    }
}
