#include <iostream>

#include "App.h"

int main() {
    App app{};

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;

        return EXIT_FAILURE;
    }

    std::cout << "Bye :p" << std::endl;
    return EXIT_SUCCESS;
}
