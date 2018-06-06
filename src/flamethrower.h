#ifndef FLAMETHROWER_H
#define FLAMETHROWER_H

#include <random>
#include "picture.h"

class Flamethrower
{
public:
    Flamethrower();
    void run(int argc, char *argv[]);

private:
    void burn_slow(int x, int y, YCbCrPixel &px);

    // Picture pic, overlay;
    int step;
};

#endif // FLAMETHROWER_H
