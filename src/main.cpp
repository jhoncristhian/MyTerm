#include <iostream>
#include "terminal.h"
#include "utils.h"

int main() {
    try {
        Terminal terminal;
        terminal.run();
    } catch (const std::exception& e) {
        std::cout << Colors::RED << "Error fatal: " << e.what() << Colors::RESET << std::endl;
        return 1;
    }

    return 0;
}