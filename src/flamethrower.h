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

    std::random_device random;
    std::mt19937_64 mt19937;
    std::uniform_real_distribution<double> uniform;

    // Picture pic, overlay;
    int step;
};

#endif // FLAMETHROWER_H
