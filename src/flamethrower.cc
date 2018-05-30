
#include <iostream>
#include <exception>
#include <random>
#include <cmath>

#include "flamethrower.h"
#include "picture.h"

void Flamethrower::run(int argc, char *argv[]) {
    if (argc <= 2) {
        throw std::runtime_error("usage: prog <in> <out>");
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    Picture pic(inputPath);

    // float aspect = (float)pic.get_width() / pic.get_height();
    // pic.resize(576 * aspect, 576);
    int fireResolution = pic.get_height() / 120;
    std::cout << "fireResolution: " << fireResolution << std::endl;

    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_real_distribution<double> unif;

    pic.scan([&](int x, int y, Pixel &px) {
#if 1
        // px.setCr(px.Pr + ((rand() % 3) - 1) * 8);
        // px.setCb(px.Pb + ((rand() % 3) - 1) * 8);

        if (y % fireResolution != 0) {
            return;
        }

        if (y + 8 > pic.get_height()) {
            return;
        }

        static int fire = 0;
        static int lastY = px.getY();
        unsigned char initY = px.getY();
        static int fireX = -1;

        double probability = 0.00;

        if (px.getY() - lastY > 64) {
            probability += 0.4;
        }

        if (unif(mt) < probability) {
            fireX = x;
        }

        // px.setCr(px.Pr + ((rand() % 3) - 1) * 8);
        // px.setCb(px.Pb + ((rand() % 3) - 1) * 8);

        for (int cy = 0; cy <= fireResolution; cy++) {
            if (fireX == -1 || (x - fireX) < 0) {
                break;
            }

            Pixel &cpx = pic.get_pixel(x, y + cy);
            // cpx.addCr((512 / ((x - fireX) + 1) + 2) * ((double)cy / fireResolution));
            double difference = x - fireX;
            /*
            double fire = 0.0
                - (difference / 1.0 - 16.0)
                * (difference / 1.0 - 16.0)
                + 256.0;
            */
            double fire = 512.0 / (difference + 1.0) + 2.0;

            if (fire < 0.0) {
                continue;
            }

            double multiplier = 0.0
                - ((double)cy / (fireResolution / 2.0) - 1.0)
                * ((double)cy / (fireResolution / 2.0) - 1.0)
                + 1.0;

            cpx.addCr(fire * multiplier);
        }

#if 0
        if (y <= 2) {
            // 1, 2
            px.Pb = 128;
        } else if ((y - 2) % 4 == 0) {
            // 6, 10, 14, 18, 22...
            px.Pb = pic.get_pixel(x, y - 2).Pb;
        } else if ((y - 3) % 4 == 0) {
            // 3, 7, 11, 15, 19, 23...
            px.Pr = pic.get_pixel(x, y - 2).Pr;
        } else if (y % 2 == 0) {
            // 4, 8, 12, 16, 20...
            px.Pr = pic.get_pixel(x, y - 2).Pr;
        } else {
            // 5, 9, 13, 17, 21...
            px.Pb = pic.get_pixel(x, y - 2).Pb;
        }
#endif

        lastY = initY;
#else
        px.setCr(255);
        px.setCb(64);
        px.addY(-64);
#endif
    });

    pic.save(outputPath);
}
