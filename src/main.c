
#include <stdlib.h>
#include "flamethrower.h"

int main(int argc, char **argv) {
    if (secam_init(argc, argv)) {
        return secam_run();
    }
    return EXIT_FAILURE;
}

