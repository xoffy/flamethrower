
#include <stdlib.h>
#include "secamizer.h"

int main(int argc, char **argv) {
    Secamizer *secamizer = secamizer_init(argc, argv);
    if (!secamizer) {
        return EXIT_FAILURE;
    }
    
    secamizer_run(secamizer);
    secamizer_destroy(&secamizer);

    return EXIT_SUCCESS;
}

