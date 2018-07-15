
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
    app->canvas = picture_clone(app->source);
    picture_resize(app->canvas, virtual_width, virtual_height);
    picture_rgba_to_yuva(app->canvas); // canvas should be in YUVA

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
