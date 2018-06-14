
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */

#include "flamethrower.h"
#include "picture.h"
#include "util.h"

YCbCrPicture *canvas = NULL;
const char *input = NULL;
const char *output = NULL;

int secam_init(int argc, char **argv) {
    int idx = 1;

    u_debug("secam_init()...");

    srand(time(NULL));

    for (idx = 1; idx < argc; idx++) {
        if (strcmp(argv[idx], "-i") == 0) {
            if (idx < argc - 1) {
                input = argv[++idx];
            }
        } else if (strcmp(argv[idx], "-o") == 0) {
            if (idx < argc - 1) {
                output = argv[++idx];
            }
        } else {
            u_error("Unknown argument %s.", argv[idx]);
            return 0;
        }
    }

    if (!input || !output) {
        u_error("no input or output file.");
        return 0;
    }

    canvas = ycbcr_picture_brdg_load(input);
    if (!canvas) {
        u_error("Can't open picture %s.", input);
        return 0;
    }

    return 1;
}

void secam_end(void) {
    u_debug("secam_end()...");
    if (canvas) {
        ycbcr_picture_delete(canvas);
    }
}

double random1() {
    return rand() / (double)RAND_MAX;
}

int secam_scan(YCbCrPicture *overlay, int x, int y, unsigned char *e) {
    double rndm, thrshld;
    int min_hs;
    static int point, is_blue;
    int bx, by, gain, hs /* horizontal step */;
    double diff, fire;

    rndm = 0.001;
    thrshld = 0.024;
    min_hs = 40;

    if (x == 0) {
        point = -1;
    }
    
    bx = canvas->width / overlay->width * x;
    by = canvas->height / overlay->height * y;
    if (bx == 0) {
        bx = 1;
    }
    
    diff = ycbcr_picture_get_pixel(canvas, bx, by)[0]
        - ycbcr_picture_get_pixel(canvas, bx - 1, by)[0] / 256.0;
    gain = point == -1 ? min_hs * 1.5 : x - point;
    hs = min_hs + random1() * (min_hs * 0.5);
    
    if ((diff * random1() + rndm * random1() > thrshld) && (gain > hs)) {
        point = x;
        is_blue = random1() <= 0.25;
    }
    
    if (point < 0) {
        return 0;
    }
    
    fire = (320.0 + random1() * 128.0) / (gain + 1.0) - 1.0;
    if (fire < 0) {
        /* fire is faded */
        point = -1;
        return 0;
    }
    
    if (is_blue) {
        /* Cb */
        e[1] = clamp_comp(e[1] + fire);
    } else {
        /* Cr */
        e[2] = clamp_comp(e[2] + fire);
    }
    
    return 0;
}

void secam_perform_simple(void) {
    YCbCrPicture *overlay, *merged;
    double ar; /* aspect ratio */
    int vr = 288; /* vertical resolution */

    u_debug("secam_perform_simple()...");

    if (!canvas) {
        u_error("secam_perform(): no canvas");
        return;
    }
    
    ar = (double)canvas->width / (double)canvas->height;

    overlay = ycbcr_picture_dummy(ar * vr, vr / 2);
    ycbcr_picture_scan(overlay, secam_scan);
    ycbcr_picture_brdg_resize(&overlay, canvas->width, canvas->height);
    merged = ycbcr_picture_merge(canvas, overlay);
    ycbcr_picture_brdg_write(overlay, output);
    
    
    ycbcr_picture_delete(overlay);
    ycbcr_picture_delete(merged);
}

int secam_run(void) {
    u_debug("secam_run()...");

    secam_perform_simple();
    secam_end();
    
    return EXIT_SUCCESS;
}





