
#include <iostream>
#include <exception>
#include <cmath>

#include "flamethrower.h"
#include "util.h"

Flamethrower::Flamethrower() {
    mt19937.seed(random());
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

    overlay.scan([&](int x, int y, YCbCrPixel &e) {
        static int point = -1;
        static bool isBlue = false;
        int bx = (double)canvas.get_width() / overlay.get_width() * x;
        int by = (double)canvas.get_height() / overlay.get_height() * y;
        if (bx == 0) { bx = 1; }
        double diff = (canvas.get_pixel(bx, by).Y
            - canvas.get_pixel(bx - 1, by).Y) / 256.0;
        double rndm = 0.001;
        double thrshld = 0.024;

        if (x == 0) {
            point = -1;
        }

        int gain = x - point;

        if (diff * uniform(mt19937) + rndm * uniform(mt19937) > thrshld
            && gain > uniform(mt19937) * 20)
        {
            point = x;
            isBlue = uniform(mt19937) <= 0.5;
        }

        if (point < 0) {
            return;
        }

        
        double err = 384.0 / (gain + 1.0) - 1.0;
        if (err < 0) {
            point = -1;
            return;
        }

        if (isBlue) {
            e.Cb = Util::clampComponent(e.Cb + err);
        } else {
            e.Cr = Util::clampComponent(e.Cr + err);
        }
    });
    overlay.resize(canvas.get_width(), canvas.get_height());
    canvas.merge(overlay, 128);
    canvas.save(outputPath);
}

void Flamethrower::burn_slow(int x, int y, YCbCrPixel &px) {
#if 0
    // px.addCr(((rand() % 3) - 1) * 8);
    // px.addCb(((rand() % 3) - 1) * 8);

    if (y % step != 0) {
        return;
    }

    static int lastY = px.Y;
    unsigned char initY = px.Y;
    static int fireX = -1;

    double probability = 0.00;

    if (px.Y - lastY > 64) {
        probability += 0.4;
    }

    if (uniform(mt19937) < probability) {
        fireX = x;
    }

    if (fireX == -1 || (x - fireX) < 0) {
        return;
    }

#if 0
    double multiplier = 0.0
        - ((double)cy / (step / 2.0) - 1.0)
        * ((double)cy / (step / 2.0) - 1.0)
        + 1.0;
#endif

    double difference = x - fireX;
    double fire = 512.0 / (difference + 1.0) + 2.0;

    if (fire < 0.0) {
        return;
    }

    px.Cr = Util::clampComponent(px.Cr + fire);

    lastY = initY;
#endif
}

/*
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
*/
