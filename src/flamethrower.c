
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */
#include <math.h> /* round */
#include <stdio.h> /* sscanf */
#include <unistd.h>

#include "flamethrower.h"
#include "picture.h"
#include "util.h"
#include "noise.h"

#if 0

// YCbCrPicture *template = NULL;
// YCbCrPicture *canvas = NULL;
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

    template = ycbcr_picture_brdg_load(input);
    if (!template) {
        u_error("Can't open picture %s.", input);
        return 0;
    }

    return 1;
}

void secam_end(void) {
    u_debug("secam_end()...");
    if (template) {
        ycbcr_picture_delete(template);
    }
}

double random1(void) {
    return rand() / (double)RAND_MAX;
}

#define MIN_HS  12  /* minimal horizontal step */

int secam_scan(YCbCrPicture *overlay, int x, int y, unsigned char *e) {
    static int point, ac /* affected component */;
    int gain, hs /* horizontal step */;
    double diff, fire, ethrshld;
    unsigned char *lower;

    if (x == 0) {
        point = -1;
        return 1;
    }
    
    diff = ((0.0
        + ycbcr_picture_get_pixel(canvas, x, y)[0]
        - ycbcr_picture_get_pixel(canvas, x - 1, y)[0])
        / 256.0);
    gain = point == -1 ? MIN_HS * 1.5 : x - point;
    hs = MIN_HS + random1() * (MIN_HS * 10.5);
    
    ethrshld = thrshld + (random1() * thrshld - thrshld * 0.5);
    
    if ((diff * random1() + rndm * random1() > ethrshld) && (gain > hs)) {
        point = x;
        ac = 2 - (random1() <= 0.25); /* Cb с вер. 0.25 */
    }
    
    if (point < 0) {
        return 1;
    }
    
    fire = (320.0 + random1() * 128.0) / (gain + 1.0) - 1.0;
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

void secam_perform_simple(void) {
    YCbCrPicture *overlay, *frame; // *ov_odd, *ov_even;
    double ar; /* aspect ratio */
    int vr; /* vertical resolution */
    int vw, vh; /* virtual width and height */
    char base[256], out[256];
    const char *ext;
    int i;
    

    u_debug("secam_perform_simple()...");

    canvas = ycbcr_picture_copy(template);
    vr = 448;
    ar = (double)template->width / (double)template->height;
    vw = ar * vr;
    vh = vr / 2;
    ycbcr_picture_brdg_resize(&canvas, vw, vh);

    if (!canvas) {
        u_error("secam_perform(): no canvas");
        return;
    }
    
    u_get_file_base(base, output);
    ext = u_get_file_ext(output);
    
    for (i = 0; i < anime; i++) {
        overlay = ycbcr_picture_dummy(vw, vh);
        frame = ycbcr_picture_copy(template);
        ycbcr_picture_scan(overlay, secam_scan);
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

int secam_run(void) {
    u_debug("secam_run()...");

    secam_perform_simple();
    secam_end();
    
    return EXIT_SUCCESS;
}


#endif

#define SECAM_FLAG_QUIET    0x0001

typedef struct {
    const char *input;
    const char *output;
    double rndm;
    double thrshld;
    int frames;
    int flags;
    RGBAPicture *source;
} SecamParameters;

SecamParameters parms;

int secam_init(int argc, char **argv) {
    int opt;

    parms.input = NULL;
    parms.output = NULL;
    parms.rndm = 0.001;
    parms.thrshld = 0.024;
    parms.frames = 1;
    parms.flags = 0;
    srand(time(NULL));

    while ((opt = getopt(argc, argv, "i:o:r:t:a:qh")) != -1) {
        switch (opt) {
        case 'i':
            parms.input = optarg;
            break;
        case 'o':
            parms.output = optarg;
            break;
        case 'r':
            sscanf(optarg, "%lf", &parms.rndm);
            break;
        case 't':
            sscanf(optarg, "%lf", &parms.thrshld);
            break;
        case 'a':
            sscanf(optarg, "%d", &parms.frames);
            break;
        case 'q':
            parms.flags |= SECAM_FLAG_QUIET;
            u_quiet = 1;
            break;
        case 'h':
            printf(
                "usage: %s -i [input] -o [output] [OPTIONS]\n"
                "\n"
                "-r [float] -- set random factor (def. %f)\n"
                "-t [float] -- set threshold (def. %f)\n"
                "-a [int] -- number of frames\n"
                "-q -- be quiet\n",
                argv[0], parms.rndm, parms.thrshld
            );
            break;
        default:
            u_error("unknown option -- %c", opt);
            exit(EXIT_FAILURE);
        }
    }

    if (!parms.input || !parms.output) {
        u_error("no input or output file.");
        return 0;
    }

    parms.source = picture_load(parms.input);
    if (!parms.source) {
        u_error("Can't open picture %s.", parms.input);
        return 0;
    }

    return 1;
}

#define MIN_HS  12  /* minimal horizontal step */

static RGBAPicture *_template;

int secam_scan(RGBAPicture *t, unsigned char *e, int x, int y) {
    static int point, ac /* affected component */;
    int gain, hs /* horizontal step */;
    double diff, fire, ethrshld;
    unsigned char *lower;
    unsigned char *p1, *p2;
    int y1, y2;

    if (x == 0 || x == t->width - 1) {
        point = -1;
        return 1;
    }
    
    p1 = picture_get_pixel(_template, x, y);
    p2 = picture_get_pixel(_template, x + 1, y);
    y1 = 16 + (65.738 * p1[0] / 256.0)
        + (129.057 * p1[1] / 256.0)
        + (25.064 * p1[2] / 256.0);
    y2 = 16 + (65.738 * p2[0] / 256.0)
        + (129.057 * p2[1] / 256.0)
        + (25.064 * p2[2] / 256.0);

    diff = (0.0 + y2 - y1) / 256.0;
    gain = point == -1 ? MIN_HS * 1.5 : x - point;
    hs = MIN_HS + FRAND() * (MIN_HS * 10.5);
    
    ethrshld = parms.thrshld + (FRAND() * parms.thrshld - parms.thrshld * 0.5);
    
    if ((diff * FRAND() + parms.rndm * FRAND() > ethrshld) && (gain > hs)) {
        point = x;
        ac = (FRAND() <= 0.25) ? 2 : 0; /* синий с вер. 0.25 */
    }
    
    if (point < 0) {
        return 1;
    }
    
    // u_debug("gain = %d", gain * 8);
    fire = (320.0 + FRAND() * 256.0) / (gain + 1.0) - 1.0;
    if (fire < 0.0) {
        /* fire is faded */
        point = -1;
        return 1;
    }
    
    e[3] = COLOR_CLAMP(round(fire));
    if (ac == 0) {
        e[0] = 255;
        e[2] = 96;
    } else {
        e[1] = 32;
        e[2] = 255;
    }
    
    return 1;
}

#define VERTICAL_RESOLUTION     288

void secam_perform(void) {
    RGBAPicture *template, *canvas, *frame;
    double ar; /* aspect ratio */
    int vw, vh; /* virtual width and height */

    template = picture_clone(parms.source);
    ar = (double)template->width / (double)template->height;
    vw = ar * VERTICAL_RESOLUTION;
    vh = VERTICAL_RESOLUTION / 2;
    picture_resize(template, vw, vh);

    canvas = picture_new(vw, vh);
    _template = template;
    picture_scan(canvas, secam_scan);
    picture_resize(canvas, parms.source->width, parms.source->height);

    frame = picture_merge(parms.source, canvas);
    picture_save(frame, parms.output);

    picture_delete(frame);
    picture_delete(canvas);
    picture_delete(template);
}

void secam_end(void) {
    picture_delete(parms.source);
}

int secam_run(void) {
    secam_perform();
    secam_end();

    return 0;
}