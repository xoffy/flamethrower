
#ifndef __FLAMETHROWER_H_
#define __FLAMETHROWER_H_

#include <stdbool.h>
#include "picture.h"

typedef struct {
    YCCPicture *source;
    const char *input_path;
    const char *output_path;
    const char *forced_output_format;
    double rndm;
    double thrshld;
    int frames;
    int pass_count;
    bool force_480;
} Secamizer;

Secamizer *secamizer_init(int argc, char **argv);
void secamizer_run(Secamizer *self);
void secamizer_destroy(Secamizer **selfp);

#endif

