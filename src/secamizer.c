
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

void secamizer_scan(Secamizer *self, YCCPicture *frame, int cx, int cy);
void chroma_noise_scan(YCCPicture *frame, double shift, double amp, int cx, int cy);

void print_usage(const char *appname) {
    printf(
    //   handy ruler
    //   ------------------------------------------------------------------------------
        "usage: %s [OPTIONS] <SOURCE> <OUTPUT>\n"
        "\n"
        "Available options:\n"
        "\n"
        " -r, --random <VALUE>         set randomization factor\n"
        "                                (default is %g)\n"
        " -t, --threshold <VALUE>      set threshold value\n"
        "                                (default is %g)\n"
        " -a, --frames <COUNT>         set count of frames\n"
        " -p, --passes <COUNT>         how many times to apply secamization\n"
        " -f, --output-format <FMT>    force output format (mandatory for stdout)\n"
        "                                supported formats: jpg, png, bmp, tga\n"
        " -n, --noise-amplitude <VAL>  amplitude of chroma noise (rec. 0.0 to 128.0)\n"
        "\n"
        " -q, --quiet                  be quiet, do not print anything\n"
        " -R, --force-480              force 480p\n"
        " -h, -?                       show this help\n"
        "\n"
        "A source can be in JPG or PNG formats. An output is same too.\n"
        "You can set source, output or both to `-` (stdin and stdout respectively).\n",
        appname, DEF_RNDM, DEF_THRSHLD
    );
}

enum Option {
    OPTION_NONE = 0,
    OPTION_RANDOMIZATION_FACTOR,
    OPTION_THRESHOLD,
    OPTION_FRAME_COUNT,
    OPTION_PASS_COUNT,
    OPTION_FORCE_FORMAT,
    OPTION_NOISE_AMPLITUDE
};

void parse_arguments(Secamizer *self, int argc, char **argv) {
    enum Option catch_option = OPTION_NONE;

    for (int i = 1; i < argc; i++) {
        if (catch_option) {
            switch (catch_option) {
            case OPTION_NONE:
                // for GCC's sake [sry for bad code]
                break;
            case OPTION_RANDOMIZATION_FACTOR:
                sscanf(argv[i], "%lf", &self->rndm);
                break;
            case OPTION_THRESHOLD:
                sscanf(argv[i], "%lf", &self->thrshld);
                break;
            case OPTION_FRAME_COUNT:
                sscanf(argv[i], "%d", &self->frames);
                break;
            case OPTION_PASS_COUNT:
                sscanf(argv[i], "%d", &self->pass_count);
                break;
            case OPTION_FORCE_FORMAT:
                self->forced_output_format = argv[i];
                break;
            case OPTION_NOISE_AMPLITUDE:
                sscanf(argv[i], "%lf", &self->noise_amplitude);
                break;
            }
            catch_option = OPTION_NONE;
            continue;
        }

        const char *arg = &argv[i][1];

        if (strcmp(arg, "q") == 0 || strcmp(arg, "-quiet") == 0) {
            u_quiet = 1;
            continue;
        } else if (strcmp(arg, "R") == 0 || strcmp(arg, "-force-480") == 0) {
            self->force_480 = true;
            continue;
        } else if (strcmp(arg, "?") == 0 || strcmp(arg, "h") == 0 || strcmp(arg, "-help") == 0) {
            print_usage(argv[0]);
            exit(0);
            continue;
        } else if (strcmp(arg, "r") == 0 || strcmp(arg, "-random") == 0) {
            catch_option = OPTION_RANDOMIZATION_FACTOR;
            continue;
        } else if (strcmp(arg, "t") == 0 || strcmp(arg, "-threshold") == 0) {
            catch_option = OPTION_THRESHOLD;
            continue;
        } else if (strcmp(arg, "a") == 0 || strcmp(arg, "-frames") == 0) {
            catch_option = OPTION_FRAME_COUNT;
            continue;
        } else if (strcmp(arg, "p") == 0 || strcmp(arg, "-passes") == 0) {
            catch_option = OPTION_PASS_COUNT;
            continue;
        } else if (strcmp(arg, "f") == 0 || strcmp(arg, "-output-format") == 0) {
            catch_option = OPTION_FORCE_FORMAT;
            continue;
        } else if (strcmp(arg, "n") == 0 || strcmp(arg, "-noise-amplitude") == 0) {
            catch_option = OPTION_NOISE_AMPLITUDE;
            continue;
        } else if (argv[i][0] == '-' && strlen(arg) > 0) {
            print_usage(argv[0]);
            u_error("Bad argument: \"%s\"", argv[i]);
            secamizer_destroy(&self);
            exit(1);
        }

        if (!self->input_path) {
            self->input_path = argv[i];
        } else if (!self->output_path) {
            self->output_path = argv[i];
        } else {
            print_usage(argv[0]);
            u_error("Can't recognize argument \"%s\"", argv[i]);
            secamizer_destroy(&self);
            exit(1);
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

    self->rndm = DEF_RNDM;
    self->thrshld = DEF_THRSHLD;
    self->frames = 1;
    self->pass_count = 1;
    self->force_480 = false;
    self->forced_output_format = NULL;
    self->noise_amplitude = 36.0;

    self->input_path = NULL;
    self->output_path = NULL;

    parse_arguments(self, argc, argv);

    if (!self->input_path || !self->output_path) {
        print_usage(argv[0]);
        u_error("You must set input and output!");
        secamizer_destroy(&self);
        exit(1);
    }

    self->source = ycc_load_picture(self->input_path,
        self->force_480 ? 480 : -1);
    if (!self->source) {
        u_error("Can't open picture %s.", self->input_path);
        return NULL;
    }

    noise_init();

    return self;
}

void secamizer_run(Secamizer *self) {
    int width = self->source->width;
    int height = self->source->height;
    
    for (int i = 0; i < self->frames; i++) {
        YCCPicture *frame = ycc_new(width, height);
        ycc_copy(frame, self->source);

        for (int pass = 0; pass < self->pass_count; pass++) {
            for (int cy = 0; cy < height / 2; cy++) {
                for (int cx = 0; cx < width / 4; cx++) {
                    if (pass == 0) {
                        chroma_noise_scan(frame, i * 16.0, self->noise_amplitude, cx, cy);
                    }
                    secamizer_scan(self, frame, cx, cy);
                }
            }
        }

        if (self->frames <= 1 || strcmp(self->output_path, "-") == 0) {
            ycc_save_picture(frame, self->output_path, self->forced_output_format);
        } else {
            char output_base_name[256];
            char output_full_name[1024];

            const char *ext = u_get_file_ext(self->output_path);
            u_get_file_base(output_base_name, self->output_path);
            sprintf(output_full_name, "%s-%d.%s", output_base_name, i, ext);
            ycc_save_picture(frame, output_full_name, self->forced_output_format);
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

void secamizer_scan(Secamizer *self, YCCPicture *frame, int cx, int cy) {
    // This guy should be rewritten completely.
    // I have to figure out how to do this thing in more
    // mathy and "analog-like" way (using noise or something like that).
    // The current discrete method is ugly.

    static int point;
    static bool is_blue;

    if (cx == 0) {
        point = -1;
        return;
    }

    uint8_t *luma = frame->luma + ((cy * 2) * frame->width + (cx * 4));

    double a = ((double)luma[0] + (double)luma[1]) / 2.0;
    double b = ((double)luma[2] + (double)luma[3]) / 2.0;
    double delta = (a - b) / 256.0;
    int gain = point == -1 ? MIN_HS * 1.5 : cx - point;
    int hs = MIN_HS + FRAND() * (MIN_HS * 10.5);
    
    double ethrshld = self->thrshld +
        (FRAND() * self->thrshld - self->thrshld * 0.5);
    
    if ((delta * FRAND() + self->rndm * FRAND() > ethrshld) && (gain > hs)) {
        point = cx;
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

    int chroma_idx = cy * (frame->width / 4) + cx;

    if (is_blue) {
        frame->cb[chroma_idx] = COLOR_CLAMP(frame->cb[chroma_idx] + fire);
    } else {
        frame->cr[chroma_idx] = COLOR_CLAMP(frame->cr[chroma_idx] + fire);
    }
}

void chroma_noise_scan(YCCPicture *frame, double shift, double amp, int cx, int cy) {
    // Using noise function here looks too redundant.
    // Simple random value would be enough and much faster.
    // ...or wouldn't?

	int chroma_idx = (cy * (frame->width / 4)) + cx;

    // must alternate noise scale (0.67~0.99) so it will not repeat
    double scale = noise2(cx * cy, 0.32, 0.2) + 0.67;

	double cbnoise = noise2(cx + 16384 * cy + shift, amp, scale) - (amp * 0.5);
	double crnoise = noise2(cx + 32768 * cy + shift, amp, scale) - (amp * 0.5);

	frame->cb[chroma_idx] = COLOR_CLAMP(frame->cb[chroma_idx] + cbnoise);
	frame->cr[chroma_idx] = COLOR_CLAMP(frame->cr[chroma_idx] + crnoise);
}
