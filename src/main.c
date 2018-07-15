
#include <stdlib.h>
#include "flamethrower.h"

int main(int argc, char **argv) {
    Flamethrower *app = secam_init(argc, argv);
    if (!app) {
        return EXIT_FAILURE;
    }
    return secam_run(app);
}

