
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "picture.h"
#include "util.h"

unsigned char clamp_comp(int comp) {
    if (comp > 255) {
        return 255;
    } else if (comp < 0) {
        return 0;
    }
    return comp;
}

RGBPicture *rgb_picture_new(void) {
    RGBPicture *rgb;

    u_debug("rgb_picture_new()...");
    
    rgb = (RGBPicture *)calloc(1, sizeof(RGBPicture));
    if (!rgb) {
        u_error("Can't allocate memory for RGBPicture struct!");
        return NULL;
    }
    
    u_debug("-- 0x%X", rgb);
    
    return rgb;
}

RGBPicture *rgb_picture_new_dummy(int width, int height, int components) {
    RGBPicture *rgb;
    int n;
    
    u_debug("rgb_picture_new_dummy(%d, %d, %d)...", width, height, components);
    
    n = width * height * components;
    rgb = rgb_picture_new();
    if (!rgb) {
        return NULL;
    }
    rgb->width = width;
    rgb->height = height;
    rgb->components = components;
    rgb->data = (unsigned char *)malloc(sizeof(unsigned char) * n);
    if (!rgb->data) {
        u_error("Can't allocate memory for RGBPicture data!");
        return NULL;
    }
    memset(rgb->data, 128, n);
    
    return rgb;
}

RGBPicture *rgb_picture_load(const char *path) {
    RGBPicture *rgb;

    u_debug("rgb_picture_load(%s)...", path);

    rgb = rgb_picture_new();
    if (!rgb) {
        return NULL;
    }
    rgb->data = stbi_load(path, &rgb->width, &rgb->height, &rgb->components, 0);
    if (!rgb->data) {
        u_error("Can't read %s!", path);
        return NULL;
    }
    
    return rgb;
}

#define RGB_DEFAULT_COMPONENTS 3

RGBPicture *rgb_picture_from_ycbcr(const YCbCrPicture *ycbcr) {
    RGBPicture *rgb;
    int n, i;

    u_debug("rgb_picture_from_ycbcr(0x%X)...", ycbcr);

    n = ycbcr->width * ycbcr->height * RGB_DEFAULT_COMPONENTS;
    rgb = rgb_picture_new();
    if (!rgb) {
        return NULL;
    }
    rgb->data = (unsigned char *)malloc(sizeof(unsigned char) * n);
    if (!rgb->data) {
        u_error("Can't allocate memory for RGBPicture data!");
        return NULL;
    }
    rgb->width = ycbcr->width;
    rgb->height = ycbcr->height;
    rgb->components = RGB_DEFAULT_COMPONENTS;
    
    i = 0;
    
    while (i < n) {
        rgb->data[i + 0] = clamp_comp(0.0
            + (298.082 * ycbcr->data[i + 0] / 256.0)
            + (408.583 * ycbcr->data[i + 2] / 256.0)
            - 222.921);
        rgb->data[i + 1] = clamp_comp(0.0
            + (298.082 * ycbcr->data[i + 0] / 256.0)
            - (100.291 * ycbcr->data[i + 1] / 256.0)
            - (208.120 * ycbcr->data[i + 2] / 256.0)
            + 135.576);
        rgb->data[i + 2] = clamp_comp(0.0
            + (298.082 * ycbcr->data[i + 0] / 256.0)
            + (516.412 * ycbcr->data[i + 1] / 256.0)
            - 276.836);
        
        i += YCBCR_COMPONENTS;
    }
    
    return rgb;
}

void rgb_picture_delete(RGBPicture *rgb) {
    u_debug("rgb_picture_delete(0x%X)...", rgb);

    if (rgb->data) {
        stbi_image_free(rgb->data);
    }
    free(rgb);
    rgb = NULL;
}



#define JPEG_QUALITY    95
#define PNG_STRIDE      0

int rgb_picture_write(const RGBPicture *rgb, const char *path) {
    const char *ext;
    int rc = 0;
    
    u_debug("rgb_picture_write(0x%X, %s)...", rgb, path);
    
    ext = u_get_file_ext(path);
    
    if (!ext) {
        u_error("Please provide output extension!");
    } else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        rc = stbi_write_jpg(path, rgb->width, rgb->height,
            rgb->components, rgb->data, JPEG_QUALITY);
    } else if (strcmp(ext, "png") == 0) {
        rc = stbi_write_png(path, rgb->width, rgb->height,
            rgb->components, rgb->data, PNG_STRIDE);
    } else if (strcmp(ext, "bmp") == 0) {
        rc = stbi_write_bmp(path, rgb->width, rgb->height,
            rgb->components, rgb->data);
    } else if (strcmp(ext, "tga") == 0) {
        rc = stbi_write_tga(path, rgb->width, rgb->height,
            rgb->components, rgb->data);
    } else {
        u_error("Unknown output extension %s!", ext);
    }
    
    return rc;
}

void rgb_picture_resize(RGBPicture *rgb, int nw, int nh) {
    unsigned char *output;

    output = malloc(sizeof(unsigned char) * nw * nh * rgb->components);

    stbir_resize_uint8(
        rgb->data, rgb->width, rgb->height, 0,
        output, nw, nh, 0, rgb->components
    );
    free(rgb->data);
    
    rgb->data = output;
    rgb->width = nw;
    rgb->height = nh;
}


YCbCrPicture *ycbcr_picture_new(void) {
    YCbCrPicture *ycbcr;
    
    u_debug("ycbcr_picture_new()...");
    
    ycbcr = (YCbCrPicture *)calloc(1, sizeof(YCbCrPicture));
    if (!ycbcr) {
        u_error("Can't allocate memory for YCbCrPicture struct!");
        return NULL;
    }
    
    u_debug("-- 0x%X", ycbcr);
    
    return ycbcr;
}



void ycbcr_picture_reset(YCbCrPicture *ycbcr) {
    int n = ycbcr->width * ycbcr->height * YCBCR_COMPONENTS;
    memset(ycbcr->data, 128, n);
}

YCbCrPicture *ycbcr_picture_dummy(int width, int height) {
    YCbCrPicture *ycbcr;
    int n;
    
    u_debug("ycbcr_picture_dummy(%d, %d)...", width, height);
    
    n = width * height * YCBCR_COMPONENTS;
    ycbcr = ycbcr_picture_new();
    if (!ycbcr) {
        return NULL;
    }
    ycbcr->width = width;
    ycbcr->height = height;
    ycbcr->data = (unsigned char *)malloc(sizeof(unsigned char) * n);
    if (!ycbcr->data) {
        u_error("Can't allocate memory for YCbCrPicture data!");
        return NULL;
    }
    memset(ycbcr->data, 128, n);
    
    return ycbcr;
}

YCbCrPicture *ycbcr_picture_from_rgb(const RGBPicture *rgb) {
    YCbCrPicture *ycbcr;
    int n, y, r;

    u_debug("ycbcr_picture_from_rgb(0x%X)...", rgb);

    if (!rgb) {
        return NULL;
    }

    n = rgb->width * rgb->height * YCBCR_COMPONENTS;
    ycbcr = ycbcr_picture_new();
    if (!ycbcr) {
        return NULL;
    }
    ycbcr->data = (unsigned char *)malloc(sizeof(unsigned char) * n);
    if (!ycbcr->data) {
        u_error("Can't allocate memory for YCbCrPicture data!");
        return NULL;
    }
    ycbcr->width = rgb->width;
    ycbcr->height = rgb->height;
    
    y = r = 0;
    
    while (y < n) {
        ycbcr->data[y + 0] = clamp_comp(16.0
            + (65.7380 * rgb->data[r + 0] / 256.0)
            + (129.057 * rgb->data[r + 1] / 256.0)
            + (25.0640 * rgb->data[r + 2] / 256.0));
        ycbcr->data[y + 1] = clamp_comp(128.0
            - (37.9450 * rgb->data[r + 0] / 256.0)
            - (74.4940 * rgb->data[r + 1] / 256.0)
            + (112.439 * rgb->data[r + 2] / 256.0));
        ycbcr->data[y + 2] = clamp_comp(128.0
            + (112.439 * rgb->data[r + 0] / 256.0)
            - (94.1540 * rgb->data[r + 1] / 256.0)
            - (18.2850 * rgb->data[r + 2] / 256.0));
        
        y += YCBCR_COMPONENTS;
        r += rgb->components;
    }
    
    return ycbcr;
}

#define PIXEL(p, x, y, c) (p->width * (c) * (y) + (c) * (x))

unsigned char *ycbcr_picture_get_pixel(const YCbCrPicture *ycbcr, int x, int y)
{ 
    return ycbcr->data + (ycbcr->width * YCBCR_COMPONENTS * y
        + YCBCR_COMPONENTS * x);
}

void ycbcr_picture_scan(YCbCrPicture *ycbcr,
    int (*func)(YCbCrPicture *, int x, int y, unsigned char *))
{
    int x, y;
    
    u_debug("ycbcr_picture_scan(0x%X)...", ycbcr);
    
    for (y = 0; y < ycbcr->height; y++) {
        for (x = 0; x < ycbcr->width; x++) {
            /* FIXME: is this shouldn't return any value? */
            func(ycbcr, x, y, ycbcr_picture_get_pixel(ycbcr, x, y));
        }
    }
}

YCbCrPicture *ycbcr_picture_copy(YCbCrPicture *orig) {
    YCbCrPicture *ycbcr;
    int n;
    
    ycbcr = ycbcr_picture_new();
    if (!ycbcr) {
        return NULL;
    }
    
    ycbcr->width = orig->width;
    ycbcr->height = orig->height;
    n = ycbcr->width * ycbcr->height * 3;
    ycbcr->data = (unsigned char *)malloc(sizeof(unsigned char) * n);
    if (!ycbcr->data) {
        u_error("Can't allocate memory for YCbCrPicture data!");
        return NULL;
    }
    memcpy(ycbcr->data, orig->data, n);
    
    return ycbcr;
}

void ycbcr_picture_merge(YCbCrPicture *ycbcr, YCbCrPicture *add) {
    int x, y, max_w, max_h, p;
    
    u_debug("ycbcr_picture_merge(0x%X, 0x%X)", ycbcr, add);
    
    max_w = ycbcr->width <= add->width ? ycbcr->width : add->width;
    max_h = ycbcr->height <= add->height ? ycbcr->height : add->height;
    
    for (y = 0; y < max_h; y++) {
        for (x = 0; x < max_w; x++) {
            p = PIXEL(ycbcr, x, y, YCBCR_COMPONENTS);
            ycbcr->data[p + 0] = clamp_comp(ycbcr->data[p + 0]
                + add->data[p + 0] - 128);
            ycbcr->data[p + 1] = clamp_comp(ycbcr->data[p + 1]
                + add->data[p + 1] - 128);
            ycbcr->data[p + 2] = clamp_comp(ycbcr->data[p + 2]
                + add->data[p + 2] - 128);
        }
    }
}

void ycbcr_picture_delete(YCbCrPicture *ycbcr) {
    u_debug("ycbcr_picture_delete(0x%X)...", ycbcr);

    if (ycbcr) {
        if (ycbcr->data) {
            free(ycbcr->data);
        }
        free(ycbcr);
        ycbcr = NULL;
    }
}

YCbCrPicture *ycbcr_picture_brdg_load(const char *path) {
    RGBPicture *bridge;
    YCbCrPicture *ycbcr;
    
    bridge = rgb_picture_load(path);
    ycbcr = ycbcr_picture_from_rgb(bridge);
    rgb_picture_delete(bridge);
    
    return ycbcr;
}

void ycbcr_picture_brdg_write(YCbCrPicture *ycbcr, const char *path) {
    RGBPicture *bridge;
    
    bridge = rgb_picture_from_ycbcr(ycbcr);
    rgb_picture_write(bridge, path);
    rgb_picture_delete(bridge);
}

void ycbcr_picture_brdg_resize(YCbCrPicture **ycbcr, int nw, int nh) {
    RGBPicture *bridge;

    u_debug("ycbcr_picture_brdg_resize(0x%X, %d, %d)...", ycbcr, nw, nh);

    bridge = rgb_picture_from_ycbcr(*ycbcr);
    ycbcr_picture_delete(*ycbcr);
    rgb_picture_resize(bridge, nw, nh);
    *ycbcr = ycbcr_picture_from_rgb(bridge);
    rgb_picture_delete(bridge);
}



//
// -----------------------------------------------------------------------------
//

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
    int width;
    int height;
    uint8_t *rgb = stbi_load(path, &width, &height, NULL, 3);
    if (!rgb) {
        u_error("[ycbcr_load_picture] Failed to load picture with STBI: %s", path);
        return NULL;
    }

    if (width % 4 != 0) {
        width = width - (width % 4);
    }

    if (height % 2 != 0) {
        height = height - (height % 2);
    }

    YCCPicture *self = ycc_new(width, height);
    if (!self) {
        return NULL;
    }

    // Initialize luminance information
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int rgb_idx = 3 * (y * width + x);
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
    for (int y = 0; y < chroma_height; y++) {
        for (int x = 0; x < chroma_width; x++) {
            int rgb_idx = 3 * ((y * 2) * width + (x * 4));
            int chroma_idx = y * chroma_width + x;
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

bool ycc_save_picture(const YCCPicture *self, const char *path) {
    uint8_t *rgb = malloc(sizeof(uint8_t) * self->width * self->height * 3);
    if (!rgb) {
        u_error("[ycbcr_save_picture] Failed to allocate memory for RGB data!");
        return false;
    }

    for (int y = 0; y < self->height; y++) {
        for (int x = 0; x < self->width; x++) {
            int rgb_idx = y * 3 * self->width + x * 3;
            int luma_idx = y * self->width + x;
            int chroma_idx = (y / 2) * (self->width / 4) + (x / 4);
            rgb[rgb_idx + 0] = COLOR_CLAMP(0.0
                + (298.082 * self->luma[luma_idx] / 256.0)
                + (408.583 * self->cr[chroma_idx] / 256.0)
                - 222.921);
            rgb[rgb_idx + 1] = COLOR_CLAMP(0.0
                + (298.082 * self->luma[luma_idx] / 256.0)
                - (100.291 * self->cb[chroma_idx] / 256.0)
                - (208.120 * self->cr[chroma_idx] / 256.0)
                + 135.576);
            rgb[rgb_idx + 2] = COLOR_CLAMP(0.0
                + (298.082 * self->luma[luma_idx] / 256.0)
                + (516.412 * self->cb[chroma_idx] / 256.0)
                - 276.836);
        }
    }

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

    for (int y = 0; y < chroma_height; y++) {
        for (int x = 0; x < chroma_width; x++) {
            int chroma_idx = chroma_width * y + x;
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
