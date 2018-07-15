
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

#define APP_FLAG_QUIET    0x0001

#define DEFAULT_VERTICAL_RESOLUTION 288

Flamethrower *secam_init(int argc, char *argv[]) {
    Flamethrower *app;
    int opt;

    app = malloc(sizeof(Flamethrower));
    app->input_path     = NULL;
    app->output_path    = NULL;
    app->frames         = 1;
    app->rndm           = 0.001;
    app->thrshld        = 0.024;
    app->flags          = 0;
    app->resolution     = DEFAULT_VERTICAL_RESOLUTION;

    srand(time(NULL));

    while ((opt = getopt(argc, argv, "i:o:r:t:a:y:qh")) != -1) {
        switch (opt) {
        case 'i':
            app->input_path = optarg;
            break;
        case 'o':
            app->output_path = optarg;
            break;
        case 'r':
            sscanf(optarg, "%lf", &app->rndm);
            break;
        case 't':
            sscanf(optarg, "%lf", &app->thrshld);
            break;
        case 'a':
            sscanf(optarg, "%d", &app->frames);
            break;
        case 'y':
            sscanf(optarg, "%d", &app->resolution);
            break;
        case 'q':
            app->flags |= APP_FLAG_QUIET;
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
                argv[0], app->rndm, app->thrshld
            );
            break;
        default:
            u_error("unknown option: `%c`", opt);
            exit(EXIT_FAILURE);
        }
    }

    if (!app->input_path || !app->output_path) {
        u_error("no input or output file.");
        return NULL;
    }

    app->source = picture_load(app->input_path);
    if (!app->source) {
        u_error("Can't open picture %s.", app->input_path);
        return NULL;
    }

    return app;
}

#define MIN_HS  12  /* minimal horizontal step */

int secam_scan(RGBAPicture *overlay, unsigned char *rgba_pixel,
    int x, int y, void *data)
{
    static int point, is_blue;

    if (x == 0 || x == overlay->width - 1) {
        point = -1;
        return 1;
    }

    const Flamethrower *app = (const Flamethrower *)data;

    /* do not forget that canvas is in YUVA not RGBA */
    unsigned char *p1 = picture_get_pixel(app->canvas, x, y);
    unsigned char *p2 = picture_get_pixel(app->canvas, x + 1, y);

    double diff = (p2[0] - p1[0]) / 256.0;
    int gain = point == -1 ? MIN_HS * 1.5 : x - point;
    int horizontal_step = MIN_HS + FRAND() * (MIN_HS * 10.5);

    double ethrshld = app->thrshld + (FRAND() * app->thrshld - app->thrshld * 0.5);

    if ((diff * FRAND() + app->rndm * FRAND() > ethrshld)
        && (gain > horizontal_step))
    {
        point = x;
        is_blue = (FRAND() <= 0.25);
    }

    if (point < 0) {
        return 1;
    }

    double fire = 320.0 / (gain + 1.0) - 1.0;
    if (fire < 0.0) {
        /* the fire is faded */
        /* FIXME: but really it's never fade */
        point = -1;
        return 1;
    }

    unsigned char yuva_pixel[4];
    yuva_pixel[0] = p1[0];
    yuva_pixel[1] = p1[1];
    yuva_pixel[2] = p1[2];
    // yuva_pixel[3] = p1[3];

    if (is_blue) {
        yuva_pixel[1] = COLOR_CLAMP(yuva_pixel[1] + round(fire));
    } else {
        yuva_pixel[2] = COLOR_CLAMP(yuva_pixel[2] + round(fire));
    }

    rgba_pixel[0] = GET_R_FROM_YUV(yuva_pixel);
    rgba_pixel[1] = GET_G_FROM_YUV(yuva_pixel);
    rgba_pixel[2] = GET_B_FROM_YUV(yuva_pixel);
    rgba_pixel[3] = COLOR_CLAMP(round(fire));

    return 1;
}

void secam_perform(Flamethrower *app) {
    double aspect_ratio =
        (double)app->source->width / (double)app->source->height;
    int virtual_width = aspect_ratio * app->resolution;
    int virtual_height = app->resolution / 2;
    RGBAPicture *rgba_canvas = picture_clone(app->source);
    picture_resize(rgba_canvas, virtual_width, virtual_height);

    /* canvas should be in YUVA */
    app->canvas = picture_new(virtual_width, virtual_height);
    for (int y = 0; y < virtual_height; y++) {
        for (int x = 0; x < virtual_width; x++) {
            unsigned char *yuva_pixel = picture_get_pixel(app->canvas, x, y);
            unsigned char *rgba_pixel = picture_get_pixel(rgba_canvas, x, y);

            yuva_pixel[0] = GET_Y_FROM_RGB(rgba_pixel);
            yuva_pixel[1] = GET_U_FROM_RGB(rgba_pixel);
            yuva_pixel[2] = GET_V_FROM_RGB(rgba_pixel);
            yuva_pixel[3] = rgba_pixel[3];
        }
    }

    picture_delete(rgba_canvas);

    for (int i = 0; i < app->frames; i++) {
        RGBAPicture *overlay = picture_new(virtual_width, virtual_height);
        picture_scan(overlay, secam_scan, (void *)app);
        picture_resize(overlay, app->source->width, app->source->height);
        RGBAPicture *frame = rgba_picture_merge(app->source, overlay, 1.0);

        if (app->frames == 1) {
            picture_save(frame, app->output_path);
            // picture_save(overlay, app->output_path);
        } else {
            char base[256], output[256];
            u_get_file_base(base, app->output_path);
            const char *ext = u_get_file_ext(app->output_path);
            sprintf(output, "%s-%d.%s", base, i, ext);
            picture_save(frame, output);
        }

        picture_delete(frame);
        picture_delete(overlay);
    }
}

void secam_end(Flamethrower *app) {
    picture_delete(app->canvas);
    picture_delete(app->source);
}

int secam_run(Flamethrower *app) {
    secam_perform(app);
    secam_end(app);

    return 0;
}
