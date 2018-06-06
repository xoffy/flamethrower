
#include <iostream>
#include <exception>
#include <cmath>

#include "flamethrower.h"
#include "util.h"

Flamethrower::Flamethrower() {
    
}

void Flamethrower::run(int argc, char *argv[]) {
    using namespace std::placeholders;

    std::string inputPath, outputPath;

    if (argc == 3) {
        inputPath = argv[1];
        outputPath = argv[2];
    } else {
        inputPath = "test.jpg";
        outputPath = "test-out.jpg";
    }

    YCbCrPicture canvas(inputPath);
    double aspect = (double)canvas.get_width() / canvas.get_height();
    int reso = 288;
    YCbCrPicture overlay(aspect * reso, reso / 2);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> urd;

    overlay.scan([&](int x, int y, YCbCrPixel &e) {
        const double rndm = 0.001;
        const double thrshld = 0.024;
        const int min_hz_step = 40;

        static int point;
        static bool is_blue = false;

        if (x == 0) {
            // point by X where the last 'shoot' was emit
            point = -1;
        }

        int bx = (double)canvas.get_width() / overlay.get_width() * x;
        int by = (double)canvas.get_height() / overlay.get_height() * y;
        if (bx == 0) {
            bx = 1;
        }
        double diff = (canvas.get_pixel(bx, by).Y
            - canvas.get_pixel(bx - 1, by).Y) / 256.0;

        int gain = point == -1
            ? min_hz_step * 1.5
            : x - point;
        int hz_step = min_hz_step + urd(mt) * (min_hz_step * 0.5);

        if (diff * urd(mt) + rndm * urd(mt) > thrshld
            && gain > hz_step)
        {
            point = x;
            is_blue = urd(mt) <= 0.25;
        }

        if (point < 0) {
            return;
        }
        
        double fire = (320.0 + urd(mt) * 128) / (gain + 1.0) - 1.0;
        if (fire < 0) {
            // fire is faded
            point = -1;
            return;
        }

        // at this point which stripe will be fired is totally random
        if (is_blue) {
            e.Cb = Util::clampComponent(e.Cb + fire);
        } else {
            e.Cr = Util::clampComponent(e.Cr + fire);
        }
    });
    overlay.resize(canvas.get_width(), canvas.get_height());
    canvas.merge(overlay);
    canvas.save(outputPath);
}

#if 0
    if (y <= 2) {
        // 1, 2
    } else if ((y - 2) % 4 == 0) {
        // 6, 10, 14, 18, 22...
    } else if ((y - 3) % 4 == 0) {
        // 3, 7, 11, 15, 19, 23...
    } else if (y % 2 == 0) {
        // 4, 8, 12, 16, 20...
    } else {
        // 5, 9, 13, 17, 21...
    }
#endif
