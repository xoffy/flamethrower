
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
    int x, y, rc;
    
    u_debug("ycbcr_picture_scan(0x%X)...", ycbcr);
    
    for (y = 0; y < ycbcr->height; y++) {
        for (x = 0; x < ycbcr->width; x++) {
            rc = func(ycbcr, x, y, ycbcr_picture_get_pixel(ycbcr, x, y));
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

    if (ycbcr->data) {
        free(ycbcr->data);
    }
    free(ycbcr);
    ycbcr = NULL;
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



