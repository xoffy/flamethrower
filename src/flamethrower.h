
#ifndef __FLAMETHROWER_H_
#define __FLAMETHROWER_H_

#include "picture.h"

typedef struct {
    const char  *input_path;
    const char  *output_path;
    RGBAPicture *source;
    YUVAPicture *canvas;
    double      rndm;
    double      thrshld;
    int         frames;
    int         flags;
    int         resolution;
} Flamethrower;

Flamethrower    *secam_init(int argc, char *argv[]);
int             secam_run(Flamethrower *app);

#endif

