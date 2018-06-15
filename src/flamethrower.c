
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */
#include <math.h> /* round */
#include <stdio.h> /* sscanf */

#include "flamethrower.h"
#include "picture.h"
#include "util.h"

YCbCrPicture *canvas = NULL;
const char *input = NULL;
const char *output = NULL;
double rndm;
double thrshld;
int anime = 1;

int secam_init(int argc, char **argv) {
    int idx = 1;

    srand(time(NULL));

    rndm = 0.001;
    thrshld = 0.024;

    for (idx = 1; idx < argc; idx++) {
        if (strcmp(argv[idx], "-i") == 0) {
            if (idx < argc - 1) {
                input = argv[++idx];
            }
        } else if (strcmp(argv[idx], "-o") == 0) {
            if (idx < argc - 1) {
                output = argv[++idx];
            }
        } else if (strcmp(argv[idx], "-r") == 0) {
            if (idx < argc - 1) {
                sscanf(argv[++idx], "%lf", &rndm);
            }
        } else if (strcmp(argv[idx], "-t") == 0) {
            if (idx < argc - 1) {
                sscanf(argv[++idx], "%lf", &thrshld);
            }
        } else if (strcmp(argv[idx], "-q") == 0) {
            u_quiet = 1;
        } else if (strcmp(argv[idx], "-a") == 0) {
            if (idx < argc - 1) {
                sscanf(argv[++idx], "%d", &anime);
            }
        } else if (strcmp(argv[idx], "-h") == 0) {
            printf(
                "usage: %s -i [input] -o [output] [OPTIONS]\n"
                "\n"
                "-r [float] -- set random factor (def. %f)\n"
                "-t [float] -- set threshold (def. %f)\n"
                "-a [int] -- number of frames\n"
                "-q -- be quiet\n",
                argv[0], rndm, thrshld
            );
            return 0;
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

double random1(void) {
    return rand() / (double)RAND_MAX;
}

#define MIN_HS  12  /* minimal horizontal step */

int secam_scan(YCbCrPicture *overlay, int x, int y, unsigned char *e) {
    static int point, is_blue;
    int cx, cy, gain, hs /* horizontal step */;
    double diff, fire, ethrshld;

    if (x == 0) {
        point = -1;
    }
    
    cx = round((double)canvas->width / overlay->width * x);
    cy = round((double)canvas->height / overlay->height * y);
    if (cx == 0) {
        cx = 2;
    }
    
    diff = ((0.0
        + ycbcr_picture_get_pixel(canvas, cx, cy)[0]
        - ycbcr_picture_get_pixel(canvas, cx - 2, cy)[0])
        / 256.0);
    gain = point == -1 ? MIN_HS * 1.5 : x - point;
    hs = MIN_HS + random1() * (MIN_HS * 10.5);
    
    ethrshld = thrshld + (random1() * thrshld - thrshld * 0.5);
    
    if ((diff * random1() + rndm * random1() > ethrshld) && (gain > hs)) {
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
    YCbCrPicture *overlay, *ov_odd, *ov_even, *frame;
    double ar; /* aspect ratio */
    int vr = 240; /* vertical resolution */
    char base[256], out[256];
    const char *ext;
    int i, ov_w, ov_h;

    u_debug("secam_perform_simple()...");

    if (!canvas) {
        u_error("secam_perform(): no canvas");
        return;
    }
    
    u_get_file_base(base, output);
    ext = u_get_file_ext(output);
    
    ar = (double)canvas->width / (double)canvas->height;
    ov_w = ar * vr;
    ov_h = vr / 2;
    
    ov_even = ycbcr_picture_dummy(canvas->width, canvas->height);
    
    for (i = 0; i < anime; i++) {
        frame = ycbcr_picture_copy(canvas);
    
        if (i % 2 == 0) {
            ov_odd = ycbcr_picture_dummy(ov_w, ov_h);
            ycbcr_picture_scan(ov_odd, secam_scan);
            ycbcr_picture_brdg_resize(&ov_odd, canvas->width, canvas->height);
        } else {
            ov_even = ycbcr_picture_dummy(ov_w, ov_h);
            ycbcr_picture_scan(ov_even, secam_scan);
            ycbcr_picture_brdg_resize(&ov_even, canvas->width, canvas->height);
        }
        
        ycbcr_picture_merge(frame, ov_odd);
        ycbcr_picture_merge(frame, ov_even);
        
        sprintf(out, "%s.%d.%s", base, i, ext);
        ycbcr_picture_brdg_write(frame, out);
        
        ycbcr_picture_delete(frame);
        
        if (i % 2 == 0) {
            ycbcr_picture_delete(ov_even);
        } else {
            ycbcr_picture_delete(ov_odd);
        }
    }
}

int secam_run(void) {
    u_debug("secam_run()...");

    secam_perform_simple();
    secam_end();
    
    return EXIT_SUCCESS;
}





