
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "picture.h"
#include "util.h"

RGBPicture *rgb_picture_create(void) {
    RGBPicture *rgb;

    rgb = (RGBPicture *)сalloc(1, sizeof(RGBPicture));
    if (!self) {
        u_error("Can't allocate memory for RGBPicture struct!");
    }
    
    return rgb;
}

RGBPicture *rgb_picture_load(const char *path) {
    RGBPicture *rgb;

    rgb = rgb_picture_create();
    rgb->data = stbi_load(path, &rgb->width, &rgb->height, &rgb->components, 0);
    
    return rgb;
}

void rgb_picture_destroy(RGBPicture *rgb) {
    if (rgb->data) {
        stbi_image_free(rgb->data);
    }
    free(rgb);
}

/* ---- ---- ---- ---- */

YCbCrPicture *ycbcr_picture_create(void) {
    YCbCrPicture *ycbcr;
    
    ycbcr = (YCbCrPicture *)calloc(1, sizeof(YCbCrPicture));
    if (!self) {
        u_error("Can't allocate memory for YCbCrPicture struct!");
    }
    
    return ycbcr;
}


YCbCrPicture *ycbcr_picture_from_rgb(RGBPicture *rgb) {
    YCbCrPicture *ycbcr;
    int area, i;

    ycbcr = ycbcr_picture_create();
    area = rgb->width * rgb->height;
    ycbcr->data = (Byte *)malloc(sizeof(Byte) * area * 3);
    
    for (i = 0; i < area; i++) {
        ycbcr->data[i + 0] = ;
        ycbcr->data[i + 1] = ;
        ycbcr->data[i + 2] = ;
    }
    
    return rgb;
}
