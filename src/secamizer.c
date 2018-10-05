
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */
#include <math.h> /* round */
#include <stdio.h> /* sscanf */

#include "secamizer.h"
#include "picture.h"
#include "util.h"
#include "noise.h"

typedef struct {
    YCbCrPicture *template;
    YCbCrPicture *canvas;
    const char *input_path;
    const char *output_path;
    double rndm;
    double thrshld;
    int frames;
} SecamizerParameters;

SecamizerParameters parms;

int secamizer_init(int argc, char **argv) {
    int idx = 1;

    srand(time(NULL));

    // Why? Because I don't mind diversity. There must be either arrows or dots.
    parms.rndm = 0.001;
    parms.thrshld = 0.024;
    parms.frames = 1;

    for (idx = 1; idx < argc; idx++) {
        if (strcmp(argv[idx], "-i") == 0) {
            if (idx < argc - 1) {
                parms.input_path = argv[++idx];
            }
        } else if (strcmp(argv[idx], "-o") == 0) {
            if (idx < argc - 1) {
                parms.output_path = argv[++idx];
            }
        } else if (strcmp(argv[idx], "-r") == 0) {
            if (idx < argc - 1) {
                sscanf(argv[++idx], "%lf", &parms.rndm);
            }
        } else if (strcmp(argv[idx], "-t") == 0) {
            if (idx < argc - 1) {
                sscanf(argv[++idx], "%lf", &parms.thrshld);
            }
        } else if (strcmp(argv[idx], "-q") == 0) {
            u_quiet = 1;
        } else if (strcmp(argv[idx], "-a") == 0) {
            if (idx < argc - 1) {
                sscanf(argv[++idx], "%d", &parms.frames);
            }
        } else if (strcmp(argv[idx], "-h") == 0) {
            printf(
                "usage: %s -i [input] -o [output] [OPTIONS]\n"
                "\n"
                "-r [float] -- set random factor (def. %f)\n"
                "-t [float] -- set threshold (def. %f)\n"
                "-a [int] -- number of frames\n"
                "-q -- be quiet\n",
                argv[0], parms.rndm, parms.thrshld
            );
            return 0;
        } else {
            u_error("Unknown argument %s.", argv[idx]);
            return 0;
        }
    }

    if (!parms.input_path) {
        u_error("No input file.");
        return 0;
    }

    if (!parms.output_path) {
        u_error("No output file.");
        return 0;
    }

    parms.template = ycbcr_picture_brdg_load(parms.input_path);
    if (!parms.template) {
        u_error("Can't open picture %s.", parms.input_path);
        return 0;
    }

    return 1;
}

void secamizer_end(void) {
    u_debug("secamizer_end()...");
    ycbcr_picture_delete(parms.template);
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
        + ycbcr_picture_get_pixel(parms.canvas, x, y)[0]
        - ycbcr_picture_get_pixel(parms.canvas, x - 1, y)[0])
        / 256.0);
    gain = point == -1 ? MIN_HS * 1.5 : x - point;
    hs = MIN_HS + FRAND() * (MIN_HS * 10.5);
    
    ethrshld = parms.thrshld +
        (FRAND() * parms.thrshld - parms.thrshld * 0.5);
    
    if ((diff * FRAND() + parms.rndm * FRAND() > ethrshld) && (gain > hs)) {
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

    parms.canvas = ycbcr_picture_copy(parms.template);
    vr = 448;
    ar = (double)parms.template->width / (double)parms.template->height;
    vw = ar * vr;
    vh = vr / 2;
    ycbcr_picture_brdg_resize(&parms.canvas, vw, vh);

    if (!parms.canvas) {
        u_error("secamizer_perform(): no canvas");
        return;
    }
    
    u_get_file_base(base, parms.output_path);
    ext = u_get_file_ext(parms.output_path);

    u_debug("*** PP -1 ***");
    
    for (i = 0; i < parms.frames; i++) {
        u_debug("*** PP 0 ***");
        overlay = ycbcr_picture_dummy(vw, vh);
        frame = ycbcr_picture_copy(parms.template);
        ycbcr_picture_scan(overlay, secamizer_scan);
        ycbcr_picture_brdg_resize(&overlay,
            parms.template->width, parms.template->height);
        ycbcr_picture_merge(frame, overlay);
        sprintf(out, "%s.%d.%s", base, i, ext);
        u_debug("*** PP 1: %s ***", out);
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





