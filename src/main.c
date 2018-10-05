
#include <stdlib.h>
#include "secamizer.h"

int main(int argc, char **argv) {
    if (secamizer_init(argc, argv)) {
        return secamizer_run();
    }
    return EXIT_FAILURE;
}

