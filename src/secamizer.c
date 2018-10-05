
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */
#include <math.h> /* round */
#include <stdio.h> /* sscanf */

#include "flamethrower.h"
#include "picture.h"
#include "util.h"
#include "noise.h"

YCbCrPicture *template = NULL;
YCbCrPicture *canvas = NULL;
const char *input = NULL;
const char *output = NULL;
double rndm;
double thrshld;
int anime = 1;

int secamizer_init(int argc, char **argv) {
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

    template = ycbcr_picture_brdg_load(input);
    if (!template) {
        u_error("Can't open picture %s.", input);
        return 0;
    }

    return 1;
}

void secamizer_end(void) {
    u_debug("secamizer_end()...");
    if (template) {
        ycbcr_picture_delete(template);
    }
}

#define MIN_HS  12  /* minimal horizontal step */

int secamizer_scan(YCbCrPicture *overlay, int x, int y, unsigned char *e) {
    static int point, ac /* affected component */;
    int gain, hs /* horizontal step */;
    double diff, fire, ethrshld;

    if (x == 0) {
        point = -1;
        return 1;
    }
    
    diff = ((0.0
        + ycbcr_picture_get_pixel(canvas, x, y)[0]
        - ycbcr_picture_get_pixel(canvas, x - 1, y)[0])
        / 256.0);
    gain = point == -1 ? MIN_HS * 1.5 : x - point;
    hs = MIN_HS + FRAND() * (MIN_HS * 10.5);
    
    ethrshld = thrshld + (FRAND() * thrshld - thrshld * 0.5);
    
    if ((diff * FRAND() + rndm * FRAND() > ethrshld) && (gain > hs)) {
        point = x;
        ac = 2 - (FRAND() <= 0.25); /* Cb с вер. 0.25 */
    }
    
    if (point < 0) {
        return 1;
    }
    
    fire = (320.0 + FRAND() * 128.0) / (gain + 1.0) - 1.0;
    if (fire < 0) {
        /* fire is faded */
        point = -1;
        return 1;
    }
    
    e[ac] = clamp_comp(e[ac] + fire);
#if 0
    if (y < (overlay->width - 1)) {
        lower = ycbcr_picture_get_pixel(overlay, x, y + 1);
        lower[ac] = clamp_comp(lower[ac] + fire);
    }
#endif
    
    return 1;
}

void secamizer_perform_simple(void) {
    YCbCrPicture *overlay, *frame; // *ov_odd, *ov_even;
    double ar; /* aspect ratio */
    int vr; /* vertical resolution */
    int vw, vh; /* virtual width and height */
    char base[256], out[256];
    const char *ext;
    int i;
    

    u_debug("secamizer_perform_simple()...");

    canvas = ycbcr_picture_copy(template);
    vr = 448;
    ar = (double)template->width / (double)template->height;
    vw = ar * vr;
    vh = vr / 2;
    ycbcr_picture_brdg_resize(&canvas, vw, vh);

    if (!canvas) {
        u_error("secamizer_perform(): no canvas");
        return;
    }
    
    u_get_file_base(base, output);
    ext = u_get_file_ext(output);
    
    for (i = 0; i < anime; i++) {
        overlay = ycbcr_picture_dummy(vw, vh);
        frame = ycbcr_picture_copy(template);
        ycbcr_picture_scan(overlay, secamizer_scan);
        ycbcr_picture_brdg_resize(&overlay, template->width, template->height);
        ycbcr_picture_merge(frame, overlay);
        sprintf(out, "%s.%d.%s", base, i, ext);
        ycbcr_picture_brdg_write(frame, out);
        ycbcr_picture_delete(frame);
        ycbcr_picture_delete(overlay);
    }
}

int noise_scan(YCbCrPicture *ycbcr, int x, int y, unsigned char *c) {
    if ((x == 0) && (y % 2 == 0)) {
        noise_init();
    }
    
    c[0] = clamp_comp(noise(x));
    return 1;
}

void noise_test() {
    YCbCrPicture *ycbcr;
    
    u_debug("noise_test()...");
    
    noise_init();
    noise_scale = 0.12;
    noise_amplitude = 255.0;
    
    ycbcr = ycbcr_picture_dummy(256, 64);
    ycbcr_picture_scan(ycbcr, noise_scan);
    ycbcr_picture_brdg_write(ycbcr, "noise.jpg");
}

int secamizer_run(void) {
    u_debug("secamizer_run()...");

    secamizer_perform_simple();
    secamizer_end();
    
    return EXIT_SUCCESS;
}





