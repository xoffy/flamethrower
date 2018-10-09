
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "picture.h"
#include "util.h"

#define JPEG_QUALITY    95
#define PNG_STRIDE      0

YCCPicture *ycc_new(int width, int height) {
    if (width % 4 != 0 || height % 2 != 0) {
        u_error("[ycbcr_new] Width must be divisible by 4 and height by 2");
        return NULL;
    }

    YCCPicture *self = malloc(sizeof(YCCPicture));
    if (!self) {
        u_error("[ycbcr_new] Failed to allocate YCbCrPicture structure.");
        return NULL;
    }

    self->width = width;
    self->height = height;

    size_t luma_size = self->width * self->height;
    size_t chroma_size = (self->width / 4) * (self->height / 2);

    self->luma = malloc(sizeof(uint8_t) * luma_size);
    self->cb = malloc(sizeof(uint8_t) * chroma_size);
    self->cr = malloc(sizeof(uint8_t) * chroma_size);

    if (!self->luma || !self->cb || !self->cr) {
        free(self);
        return NULL;
    }

    return self;
}

void ycc_reset(YCCPicture *self) {
    size_t luma_size = self->width * self->height;
    size_t chroma_size = (self->width / 4) * (self->height / 2);
    memset(self->luma, 128, luma_size);
    memset(self->cb, 128, chroma_size);
    memset(self->cr, 128, chroma_size);
}

YCCPicture *ycc_load_picture(const char *path) {
    int original_width;
    int original_height;
    uint8_t *rgb = stbi_load(path, &original_width, &original_height, NULL, 3);
    if (!rgb) {
        u_error("[ycbcr_load_picture] Failed to load picture with STBI: %s", path);
        return NULL;
    }

    int width = original_width - (original_width % 4);
    int height = original_height - (original_height % 2);

    YCCPicture *self = ycc_new(width, height);
    if (!self) {
        return NULL;
    }

    // Initialize luminance information
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int rgb_idx = 3 * (y * original_width + x);
            int luma_idx = y * width + x;
            self->luma[luma_idx] = COLOR_CLAMP(16.0
                + (65.7380 * rgb[rgb_idx + 0] / 256.0)
                + (129.057 * rgb[rgb_idx + 1] / 256.0)
                + (25.0640 * rgb[rgb_idx + 2] / 256.0));
        }
    }

    int chroma_width = (width / 4);
    int chroma_height = (height / 2);

    // Now it's time for chrominance.
    for (int cy = 0; cy < chroma_height; cy++) {
        for (int cx = 0; cx < chroma_width; cx++) {
            int rgb_idx = 3 * ((cy * 2) * original_width + (cx * 4));
            int chroma_idx = cy * chroma_width + cx;
            self->cb[chroma_idx] = COLOR_CLAMP(128.0
                - (37.9450 * rgb[rgb_idx + 0] / 256.0)
                - (74.4940 * rgb[rgb_idx + 1] / 256.0)
                + (112.439 * rgb[rgb_idx + 2] / 256.0));
            self->cr[chroma_idx] = COLOR_CLAMP(128.0
                + (112.439 * rgb[rgb_idx + 0] / 256.0)
                - (94.1540 * rgb[rgb_idx + 1] / 256.0)
                - (18.2850 * rgb[rgb_idx + 2] / 256.0));
        }
    }

    stbi_image_free(rgb);

    return self;
}

void ycbcr_to_rgb(uint8_t *dest, uint8_t luma, uint8_t cb, uint8_t cr) {
    dest[0] = COLOR_CLAMP(0.0
        + (298.082 * luma / 256.0)
        + (408.583 * cr / 256.0)
        - 222.921);
    dest[1] = COLOR_CLAMP(0.0
        + (298.082 * luma / 256.0)
        - (100.291 * cb / 256.0)
        - (208.120 * cr / 256.0)
        + 135.576);
    dest[2] = COLOR_CLAMP(0.0
        + (298.082 * luma / 256.0)
        + (516.412 * cb / 256.0)
        - 276.836);
}

bool ycc_save_picture(const YCCPicture *self, const char *path) {
    uint8_t *rgb = malloc(sizeof(uint8_t) * self->width * self->height * 3);
    if (!rgb) {
        u_error("[ycbcr_save_picture] Failed to allocate memory for RGB data!");
        return false;
    }

    uint8_t *delay = malloc(sizeof(uint8_t) * self->width * 2);
    if (!delay) {
        u_error("ycbcr_save_picture: failed to allocate memory for delay line!");
        return false;
    }

    for (int y = 0; y < self->height; y += 2) {
        for (int x = 0; x < self->width; x++) {
            int rgb_idx = y * 3 * self->width + x * 3;
            int luma_idx = y * self->width + x;
            int chroma_idx = (y / 2) * (self->width / 4) + (x / 4);

            //
            // horizontal chroma interpolation
            //
            double t = (double)(x % 4) / 4.0;
            uint8_t cb = LERP(self->cb[chroma_idx], self->cb[chroma_idx + 1], t);
            uint8_t cr = LERP(self->cr[chroma_idx], self->cr[chroma_idx + 1], t);
            ycbcr_to_rgb(&rgb[rgb_idx], self->luma[luma_idx], cb, cr);

            //
            // vertical chroma interpolation
            //
            if (y > 0) {
                int rgb_uidx = rgb_idx - (self->width * 3);
                int luma_uidx = luma_idx - self->width;
                uint8_t dcb = LERP(delay[x], cb, 0.5);
                uint8_t dcr = LERP(delay[x + self->width], cr, 0.5);
                ycbcr_to_rgb(&rgb[rgb_uidx], self->luma[luma_uidx], dcb, dcr);
            }
            
            //
            // memorize odd line chroma
            //
            delay[x] = cb;
            delay[x + self->width] = cr;
        }
    }

    free(delay);

    const char *ext = u_get_file_ext(path);
    bool rc = false;

    if (!ext) {
        u_error("Please provide output extension!");
    } else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        rc = stbi_write_jpg(path, self->width, self->height, 3, rgb, JPEG_QUALITY);
    } else if (strcmp(ext, "png") == 0) {
        rc = stbi_write_png(path, self->width, self->height, 3, rgb, PNG_STRIDE);
    } else if (strcmp(ext, "bmp") == 0) {
        rc = stbi_write_bmp(path, self->width, self->height, 3, rgb);
    } else if (strcmp(ext, "tga") == 0) {
        rc = stbi_write_tga(path, self->width, self->height, 3, rgb);
    } else {
        u_error("Unknown output extension %s!", ext);
    }

    free(rgb);

    return rc;
}

void ycc_copy(YCCPicture *dst, const YCCPicture *src) {
    size_t luma_size = src->width * src->height * sizeof(uint8_t);
    size_t chroma_size = (src->width / 4) * (src->height / 2) * sizeof(uint8_t);

    if (dst->width != src->width || dst->height != src->height) {
        dst->luma = realloc(dst->luma, luma_size);
        dst->cb = realloc(dst->cb, chroma_size);
        dst->cr = realloc(dst->cr, chroma_size);
    }

    memcpy(dst->luma, src->luma, luma_size);
    memcpy(dst->cb, src->cb, chroma_size);
    memcpy(dst->cr, src->cr, chroma_size);

    dst->width = src->width;
    dst->height = dst->height;
}

bool ycc_merge(YCCPicture *base, YCCPicture *add) {
    if (base->width != add->width || base->height != add->height) {
        u_error("[ycbcr_merge] Only pictures of the same size can be merged!");
        return false;
    }
    
    for (int y = 0; y < base->height; y++) {
        for (int x = 0; x < base->width; x++) {
            int luma_idx = base->width * y + x;
            base->luma[luma_idx] = COLOR_CLAMP(base->luma[luma_idx]
                + add->luma[luma_idx] - 128);
        }
    }

    int chroma_width = (base->width / 4);
    int chroma_height = (base->height / 2);

    for (int cy = 0; cy < chroma_height; cy++) {
        for (int cx = 0; cx < chroma_width; cx++) {
            int chroma_idx = chroma_width * cy + cx;
            base->cb[chroma_idx] = COLOR_CLAMP(base->cb[chroma_idx]
                + add->cb[chroma_idx] - 128);
            base->cr[chroma_idx] = COLOR_CLAMP(base->cr[chroma_idx]
                + add->cr[chroma_idx] - 128);
        }
    }

    return true;
}

void ycc_delete(YCCPicture **selfp) {
    YCCPicture *self = *selfp;

    free(self->luma);
    free(self->cb);
    free(self->cr);
    free(self);

    *selfp = NULL;
}
