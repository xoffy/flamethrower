
#include <iostream>
#include <exception>

#include "flamethrower.h"

int main(int argc, char *argv[]) {
    try {
        Flamethrower app;
        app.run(argc, argv);
    } catch (const std::exception &e) {
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

