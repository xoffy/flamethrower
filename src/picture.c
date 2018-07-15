
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "picture.h"
#include "util.h"

Picture *picture_create(void) {
    u_debug("picture_create()...");

    Picture *pic = malloc(sizeof(Picture));
    if (!pic) {
        u_critical("Can't allocate memory for RGBPicture struct!");
        return NULL;
    }

    u_debug(">> 0x%X", pic);
    return pic;
}

Picture *picture_new(int width, int height) {
    u_debug("picture_new(%d, %d)...", width, height);

    int size = width * height * PIC_COMPONENTS;
    Picture *pic = picture_create();
    pic->width = width;
    pic->height = height;
    pic->data = malloc(sizeof(unsigned char) * size);
    if (!pic->data) {
        u_error("Can't allocate memory for Picture data!");
        return NULL;
    }
    memset(pic->data, 0, size);

    return pic;
}

Picture *picture_clone(const Picture *src) {
    Picture *result = picture_new(src->width, src->height);
    int size = src->width * src->height * PIC_COMPONENTS;
    memcpy(result->data, src->data, size);

    return result;
}

Picture *picture_load(const char *path) {
    u_debug("picture_load(%s)...", path);

    Picture *pic = picture_create();
    pic->data = stbi_load(path, &pic->width, &pic->height,
        NULL, PIC_COMPONENTS);
    if (!pic->data) {
        u_error("Can't read %s!", path);
        return NULL;
    }

    return pic;
}

void picture_delete(Picture *pic) {
    u_debug("picture_delete(0x%X)...", pic);
    if (pic->data) {
        stbi_image_free(pic->data);
    }
    free(pic);
}

#define JPEG_QUALITY    95
#define PNG_STRIDE      0

int picture_save(const Picture *pic, const char *path) {
    u_debug("picture_write(0x%X, %s)...", pic, path);

    const char *ext = u_get_file_ext(path);
    int rc = 0;

    if (!ext) {
        u_error("Please provide output extension!");
    } else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        rc = stbi_write_jpg(path, pic->width, pic->height,
            PIC_COMPONENTS, pic->data, JPEG_QUALITY);
    } else if (strcmp(ext, "png") == 0) {
        rc = stbi_write_png(path, pic->width, pic->height,
            PIC_COMPONENTS, pic->data, PNG_STRIDE);
    } else if (strcmp(ext, "bmp") == 0) {
        rc = stbi_write_bmp(path, pic->width, pic->height,
            PIC_COMPONENTS, pic->data);
    } else if (strcmp(ext, "tga") == 0) {
        rc = stbi_write_tga(path, pic->width, pic->height,
            PIC_COMPONENTS, pic->data);
    } else {
        u_error("Unknown output extension %s!", ext);
    }

    return rc;
}

void picture_resize(Picture *pic, int new_width, int new_height) {
    u_debug("picture_resize(0x%X, %d, %d)...", pic, new_width, new_height);

    unsigned char *new_data = malloc(sizeof(unsigned char)
        * new_width * new_height * PIC_COMPONENTS);
    stbir_resize_uint8(
        pic->data, pic->width, pic->height, 0,
        new_data, new_width, new_height, 0, PIC_COMPONENTS
    );
    free(pic->data);

    pic->data = new_data;
    pic->width = new_width;
    pic->height = new_height;
}

unsigned char *picture_get_pixel(const Picture *pic, int x, int y) {
    return pic->data
        + y * pic->width * PIC_COMPONENTS
        + x * PIC_COMPONENTS;
}

void picture_scan(Picture *pic,
    int (*f)(Picture *, unsigned char *, int, int, void *),
    void *data)
{
    for (int y = 0; y < pic->height; y++) {
        for (int x = 0; x < pic->width; x++) {
            if (!f(pic, picture_get_pixel(pic, x, y), x, y, data)) {
                return;
            }
        }
    }
}

RGBAPicture *rgba_picture_merge(RGBAPicture *base,
    RGBAPicture *added,
    double opacity)
{
    RGBAPicture *result = picture_new(base->width, base->height);

    for (int y = 0; y < result->width; y++) {
        for (int x = 0; x < result->width; x++) {
            unsigned char *e_result = picture_get_pixel(result, x, y);
            unsigned char *e_added = picture_get_pixel(added, x, y);
            unsigned char *e_base = picture_get_pixel(base, x, y);
            double alpha = e_added[3] / 255.0 * opacity;

            e_result[0] = (alpha * e_added[0]) + ((1.0 - alpha) * e_base[0]);
            e_result[1] = (alpha * e_added[1]) + ((1.0 - alpha) * e_base[1]);
            e_result[2] = (alpha * e_added[2]) + ((1.0 - alpha) * e_base[2]);
            e_result[3] = 255;
        }
    }

    return result;
}

void picture_clear(Picture *pic) {
    memset(pic->data, 0, pic->width * pic->height * PIC_COMPONENTS);
}

void picture_rgba_to_yuva(RGBAPicture *rgba) {
    for (int y = 0; y < rgba->height; y++) {
        for (int x = 0; x < rgba->width; x++) {
            unsigned char *current_pixel = picture_get_pixel(rgba, x, y);
            unsigned char Y = GET_Y_FROM_RGB(current_pixel);
            unsigned char Cb = GET_U_FROM_RGB(current_pixel);
            unsigned char Cr = GET_V_FROM_RGB(current_pixel);
            current_pixel[0] = Y;
            current_pixel[1] = Cb;
            current_pixel[2] = Cr;
        }
    }
}

void picture_yuva_to_rgba(YUVAPicture *yuva) {
    for (int y = 0; y < yuva->height; y++) {
        for (int x = 0; x < yuva->width; x++) {
            unsigned char *current_pixel = picture_get_pixel(yuva, x, y);
            unsigned char r = GET_R_FROM_YUV(current_pixel);
            unsigned char g = GET_G_FROM_YUV(current_pixel);
            unsigned char b = GET_B_FROM_YUV(current_pixel);
            current_pixel[0] = r;
            current_pixel[1] = g;
            current_pixel[2] = b;
        }
    }
}