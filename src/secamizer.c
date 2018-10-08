
#include <stdlib.h> /* rand */
#include <time.h> /* time */
#include <string.h> /* strcmp */
#include <math.h> /* round */
#include <stdio.h> /* sscanf */
#include <stdbool.h>

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
        "    -? -h           show this help\n"
        "\n"
        "A source can be in JPG or PNG formats. An output is same too.\n",
        appname, DEF_RNDM, DEF_THRSHLD
    );
    exit(0);
}

void parse_arguments(Secamizer *self, int argc, char **argv) {
    char catch_option = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (catch_option) {
                u_error("Expected argument for option \"%c\".", catch_option);
                usage(argv[0]);
            }

            if (argv[i][2] != '\0') {
                u_error("Bad argument \"%s\"!", argv[i] + 1);
                usage(argv[0]);
            }

            switch (argv[i][1]) {
            case 'q':
                u_quiet = 1;
                break;
            case 'r':
            case 't':
            case 'a':
            case 'p':
                catch_option = argv[i][1];
                continue;
            case 'h':
            case '?':
                usage(argv[0]);
            default:
                u_error("Unknown option \"%c\".", argv[i][1]);
                usage(argv[0]);
            }
        } else if (catch_option) {
            switch (catch_option) {
            case 'r':
                sscanf(argv[i], "%lf", &self->rndm);
                break;
            case 't':
                sscanf(argv[i], "%lf", &self->thrshld);
                break;
            case 'a':
                sscanf(argv[i], "%d", &self->frames);
                break;
            case 'p':
                sscanf(argv[i], "%d", &self->pass_count);
                break;
            }
            catch_option = 0;
            continue;
        } else {
            // arguments without hyphen are treated as
            // input and output paths respectively
            if (!self->input_path) {
                self->input_path = argv[i];
            } else if (!self->output_path) {
                self->output_path = argv[i];
            } else {
                u_error("Can't recognize argument \"%s\"", argv[i]);
                usage(argv[0]);
            }
        }
    }
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
    self->pass_count = 1;

    self->input_path = NULL;
    self->output_path = NULL;

    parse_arguments(self, argc, argv);

    if (!self->input_path || !self->output_path) {
        secamizer_destroy(&self);
        usage(argv[0]);
    }

    self->source = ycc_load_picture(self->input_path);
    if (!self->source) {
        u_error("Can't open picture %s.", self->input_path);
        return NULL;
    }

    return self;
}

void secamizer_scan(Secamizer *self, YCCPicture *frame, int x, int y);

void secamizer_run(Secamizer *self) {
    char output_base_name[256];
    char output_full_name[1024];
    u_get_file_base(output_base_name, self->output_path);
    const char *ext = u_get_file_ext(self->output_path);

    int width = self->source->width;
    int height = self->source->height;
    
    for (int i = 0; i < self->frames; i++) {
        YCCPicture *frame = ycc_new(width, height);
        ycc_copy(frame, self->source);

        for (int j = 0; j < self->pass_count; j++) {
            for (int y = 0; y < height / 2; y++) {
                for (int x = 0; x < width / 4; x++) {
                    secamizer_scan(self, frame, x, y);
                }
            }
        }

        if (self->frames == 1) {
            ycc_save_picture(frame, self->output_path);
        } else {
            sprintf(output_full_name, "%s-%d.%s", output_base_name, i, ext);
            ycc_save_picture(frame, output_full_name);
        }
        ycc_delete(&frame);
    }
}

void secamizer_destroy(Secamizer **selfp) {
    Secamizer *self = *selfp;
    if (self->source) {
        ycc_delete(&self->source);
    }
    *selfp = NULL;
}

#define MIN_HS  12  /* minimal horizontal step */

void secamizer_scan(Secamizer *self, YCCPicture *frame, int x, int y) {
    static int point;
    static bool is_blue;

    if (x == 0) {
        point = -1;
        return;
    }
    
    uint8_t *luma = frame->luma + ((y * 2) * frame->width + (x * 4));

    double a = ((double)luma[0] + (double)luma[1]) / 2.0;
    double b = ((double)luma[2] + (double)luma[3]) / 2.0;
    double delta = (a - b) / 256.0;
    int gain = point == -1 ? MIN_HS * 1.5 : x - point;
    int hs = MIN_HS + FRAND() * (MIN_HS * 10.5);
    
    double ethrshld = self->thrshld +
        (FRAND() * self->thrshld - self->thrshld * 0.5);
    
    if ((delta * FRAND() + self->rndm * FRAND() > ethrshld) && (gain > hs)) {
        point = x;
        is_blue = (FRAND() <= 0.25); /* Cb с вер. 0.25 */
    }
    
    if (point < 0) {
        return;
    }
    
    double fire = (320.0 + FRAND() * 128.0) / (gain + 1.0) - 1.0;
    if (fire < 0) {
        /* fire is faded */
        point = -1;
        return;
    }

    int chroma_idx = y * (frame->width / 4) + x;

    if (is_blue) {
        frame->cb[chroma_idx] = COLOR_CLAMP(frame->cb[chroma_idx] + fire);
    } else {
        frame->cr[chroma_idx] = COLOR_CLAMP(frame->cr[chroma_idx] + fire);
    }
}
