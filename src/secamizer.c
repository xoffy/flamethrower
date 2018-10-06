
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */
#include <math.h> /* round */
#include <stdio.h> /* sscanf */

#include "secamizer.h"
#include "picture.h"
#include "util.h"
#include "noise.h"

#define DEF_RNDM 0.001
#define DEF_THRSHLD 0.024

void usage(const char *appname) {
    printf(
        "usage: %s [OPTIONS] <SOURCE> <OUTPUT>\n"
        "\n"
        "Available options:\n"
        "\n"
        "    -r <VALUE>      set randomization factor, default is %g\n"
        "    -t <VALUE>      set threshold value, default is %g\n"
        "    -a <COUNT>      set count of frames\n"
        "    -q              be quiet, do not print anything\n"
        "\n"
        "A source can be in JPG or PNG formats. An output is same too.\n",
        appname, DEF_RNDM, DEF_THRSHLD
    );
    exit(0);
}

Secamizer *secamizer_init(int argc, char **argv) {
    srand(time(NULL));

    Secamizer *self = malloc(sizeof(Secamizer));
    if (!self) {
        u_error("Failed to allocate Secamizer!");
        return NULL;
    }

    // Why? Because I don't mind diversity. There must be either arrows or dots.
    self->rndm = DEF_RNDM;
    self->thrshld = DEF_THRSHLD;
    self->frames = 1;

    self->input_path = NULL;
    self->output_path = NULL;

    char catch_argument = '\0';
    int show_usage = 0;

    for (int i = 1; i < argc; i++) {
        if (catch_argument != '\0') {
            switch (catch_argument) {
            case 'r':
                sscanf(argv[i], "%lf", &self->rndm);
                break;
            case 't':
                sscanf(argv[i], "%lf", &self->thrshld);
                break;
            case 'a':
                sscanf(argv[i], "%d", &self->frames);
                break;
            default:
                u_error("Unknown option \"%c\".", catch_argument);
                show_usage = 1;
                return NULL;
            }
            catch_argument = '\0';
            continue;
        }

        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'q':
                u_quiet = 1;
                break;
            default:
                if (i == argc - 1) {
                    u_error("Unknown switch \"%c\".", argv[i][1]);
                    show_usage = 1;
                    break;
                }
                catch_argument = argv[i][1];
                break;
            }
        } else {
            // arguments without hyphen are treated as
            // input and output paths respectively
            if (!self->input_path) {
                self->input_path = argv[i];
            } else if (!self->output_path) {
                self->output_path = argv[i];
            } else {
                u_error("Can't recognize argument \"%s\"", argv[i]);
                show_usage = 1;
                return NULL;
            }
        }
    }

    if (show_usage || !self->input_path || !self->output_path) {
        secamizer_destroy(&self);
        usage(argv[0]);
    }

    self->template = ycbcr_picture_brdg_load(self->input_path);
    if (!self->template) {
        u_error("Can't open picture %s.", self->input_path);
        return NULL;
    }

    return self;
}

int secamizer_scan(Secamizer *self, YCbCrPicture *canvas, YCbCrPicture *overlay, int x, int y, unsigned char *e);

void secamizer_run(Secamizer *self) {
    YCbCrPicture *canvas = ycbcr_picture_copy(self->template);
    int vertical_resolution = 448;
    double aspect_ratio = (double)self->template->width / (double)self->template->height;
    int virtual_width = aspect_ratio * vertical_resolution;
    int virtual_height = vertical_resolution / 2;
    ycbcr_picture_brdg_resize(&canvas, virtual_width, virtual_height);

    if (!canvas) {
        u_error("critical: no canvas");
        return;
    }
    
    char output_base_name[256];
    char output_full_name[1024];
    u_get_file_base(output_base_name, self->output_path);
    const char *ext = u_get_file_ext(self->output_path);
    
    for (int i = 0; i < self->frames; i++) {
        YCbCrPicture *overlay = ycbcr_picture_dummy(virtual_width, virtual_height);
        YCbCrPicture *frame = ycbcr_picture_copy(self->template);

        for (int y = 0; y < overlay->height; y++) {
            for (int x = 0; x < overlay->width; x++) {
                secamizer_scan(self, canvas, overlay, x, y,
                    ycbcr_picture_get_pixel(overlay, x, y));
            }
        }

        ycbcr_picture_brdg_resize(&overlay,
            self->template->width, self->template->height);
        ycbcr_picture_merge(frame, overlay);
        sprintf(output_full_name, "%s-%d.%s", output_base_name, i, ext);
        ycbcr_picture_brdg_write(frame, output_full_name);
        ycbcr_picture_delete(frame);
        ycbcr_picture_delete(overlay);
    }
}

void secamizer_destroy(Secamizer **selfp) {
    Secamizer *self = *selfp;
    ycbcr_picture_delete(self->template);
    *selfp = NULL;
}

#define MIN_HS  12  /* minimal horizontal step */

int secamizer_scan(Secamizer *self, YCbCrPicture *canvas, YCbCrPicture *overlay, int x, int y, unsigned char *e) {
    static int point, ac /* affected component */;

    if (x == 0) {
        point = -1;
        return 1;
    }
    
    double diff = ((0.0
        + ycbcr_picture_get_pixel(canvas, x, y)[0]
        - ycbcr_picture_get_pixel(canvas, x - 1, y)[0])
        / 256.0);
    int gain = point == -1 ? MIN_HS * 1.5 : x - point;
    int hs = MIN_HS + FRAND() * (MIN_HS * 10.5);
    
    double ethrshld = self->thrshld +
        (FRAND() * self->thrshld - self->thrshld * 0.5);
    
    if ((diff * FRAND() + self->rndm * FRAND() > ethrshld) && (gain > hs)) {
        point = x;
        ac = 2 - (FRAND() <= 0.25); /* Cb с вер. 0.25 */
    }
    
    if (point < 0) {
        return 1;
    }
    
    double fire = (320.0 + FRAND() * 128.0) / (gain + 1.0) - 1.0;
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







