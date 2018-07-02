
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

RGBAPicture *picture_create(void) {
    RGBAPicture *rgba;

    u_debug("picture_create()...");
    
    rgba = malloc(sizeof(RGBAPicture));
    if (!rgba) {
        u_error("Can't allocate memory for RGBPicture struct!");
        return NULL;
    }
    
    u_debug(">> 0x%X", rgba);
    return rgba;
}

RGBAPicture *picture_new(int width, int height) {
    RGBAPicture *rgba;
    int size;
    
    u_debug("picture_new(%d, %d)...", width, height);
    
    size = width * height * RGBA_COMPONENTS;
    rgba = picture_create();
    if (!rgba) {
        return NULL;
    }
    rgba->width = width;
    rgba->height = height;
    rgba->data = malloc(sizeof(unsigned char) * size);
    if (!rgba->data) {
        u_error("Can't allocate memory for RGBAPicture data!");
        return NULL;
    }
    memset(rgba->data, 0, size);
    
    return rgba;
}

RGBAPicture *picture_clone(const RGBAPicture *src) {
    RGBAPicture *result;
    unsigned int size;

    result = picture_new(src->width, src->height);
    if (!result) {
        return NULL;
    }
    size = src->width * src->height * RGBA_COMPONENTS;
    memcpy(result->data, src->data, size);

    return result;
}

RGBAPicture *picture_load(const char *path) {
    RGBAPicture *rgba;

    u_debug("picture_load(%s)...", path);

    rgba = picture_create();
    if (!rgba) {
        return NULL;
    }
    rgba->data = stbi_load(path, &rgba->width, &rgba->height,
        NULL, RGBA_COMPONENTS);
    if (!rgba->data) {
        u_error("Can't read %s!", path);
        return NULL;
    }

    return rgba;
}

void picture_delete(RGBAPicture *rgba) {
    u_debug("picture_delete(0x%X)...", rgba);

    if (rgba->data) {
        stbi_image_free(rgba->data);
    }
    free(rgba);
}

#define JPEG_QUALITY    95
#define PNG_STRIDE      0

int picture_save(const RGBAPicture *rgba, const char *path) {
    const char *ext;
    int rc = 0;
    
    u_debug("picture_write(0x%X, %s)...", rgba, path);
    
    ext = u_get_file_ext(path);
    
    if (!ext) {
        u_error("Please provide output extension!");
    } else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        rc = stbi_write_jpg(path, rgba->width, rgba->height,
            RGBA_COMPONENTS, rgba->data, JPEG_QUALITY);
    } else if (strcmp(ext, "png") == 0) {
        rc = stbi_write_png(path, rgba->width, rgba->height,
            RGBA_COMPONENTS, rgba->data, PNG_STRIDE);
    } else if (strcmp(ext, "bmp") == 0) {
        rc = stbi_write_bmp(path, rgba->width, rgba->height,
            RGBA_COMPONENTS, rgba->data);
    } else if (strcmp(ext, "tga") == 0) {
        rc = stbi_write_tga(path, rgba->width, rgba->height,
            RGBA_COMPONENTS, rgba->data);
    } else {
        u_error("Unknown output extension %s!", ext);
    }
    
    return rc;
}

void picture_resize(RGBAPicture *rgba, int new_width, int new_height) {
    unsigned char *output;

    u_debug("picture_resize(0x%X, %d, %d)...", rgba, new_width, new_height);

    output = malloc(sizeof(unsigned char)
        * new_width * new_height * RGBA_COMPONENTS);
    stbir_resize_uint8(
        rgba->data, rgba->width, rgba->height, 0,
        output, new_width, new_height, 0, RGBA_COMPONENTS
    );
    free(rgba->data);
    
    rgba->data = output;
    rgba->width = new_width;
    rgba->height = new_height;
}

unsigned char *picture_get_pixel(RGBAPicture *rgba, int x, int y) {
    return rgba->data
        + y * rgba->width * RGBA_COMPONENTS
        + x * RGBA_COMPONENTS;
}

void picture_scan(RGBAPicture *rgba,
    int (*f)(RGBAPicture *, unsigned char *, int, int))
{
    int x, y, rc;

    for (y = 0; y < rgba->height; y++) {
        for (x = 0; x < rgba->width; x++) {
            rc = f(rgba, picture_get_pixel(rgba, x, y), x, y);
            if (!rc) {
                return;
            }
        }
    }
}

static RGBAPicture *_merge_base, *_merge_added;

static int _picture_merge(RGBAPicture *rgba, unsigned char *e, int x, int y) {
    unsigned char *e_added, *e_base;
    double alpha;

    e_added = picture_get_pixel(_merge_added, x, y);
    e_base = picture_get_pixel(_merge_base, x, y);
    alpha = e_added[3] / 255.0;

    e[3] = 255;
    e[0] = (alpha * e_added[0]) + ((1.0 - alpha) * e_base[0]);
    e[1] = (alpha * e_added[1]) + ((1.0 - alpha) * e_base[1]);
    e[2] = (alpha * e_added[2]) + ((1.0 - alpha) * e_base[2]);

    return 1;
}

RGBAPicture *picture_merge(RGBAPicture *base, RGBAPicture *added) {
    RGBAPicture *result;

    result = picture_new(base->width, base->height);
    if (!result) {
        return NULL;
    }

    _merge_base = base;
    _merge_added = added;
    picture_scan(result, _picture_merge);

    return result;
}

/*
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
    
ycbcr->data[p + 0] = clamp_comp(ycbcr->data[p + 0]
    + add->data[p + 0] - 128);
ycbcr->data[p + 1] = clamp_comp(ycbcr->data[p + 1]
    + add->data[p + 1] - 128);
ycbcr->data[p + 2] = clamp_comp(ycbcr->data[p + 2]
    + add->data[p + 2] - 128);
*/