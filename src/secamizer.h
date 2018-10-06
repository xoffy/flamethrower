
#ifndef __FLAMETHROWER_H_
#define __FLAMETHROWER_H_

#include "picture.h"

typedef struct {
    YCbCrPicture *template;
    YCbCrPicture *canvas;
    const char *input_path;
    const char *output_path;
    double rndm;
    double thrshld;
    int frames;
    int pass_count;
} Secamizer;

Secamizer *secamizer_init(int argc, char **argv);
void secamizer_run(Secamizer *self);
void secamizer_destroy(Secamizer **selfp);

#endif

