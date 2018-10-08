
#ifndef __PICTURE_H_
#define __PICTURE_H_

#include <stdint.h>
#include <stdbool.h>

#define COLOR_CLAMP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

typedef struct {
    uint8_t     *luma; // width * height
    uint8_t     *cb; // (width / 4) * (height / 2)
    uint8_t     *cr; // (width / 4) * (height / 2)
    int         width;
    int         height;
} YCCPicture;

YCCPicture *ycc_new(int width, int height);
void ycc_reset(YCCPicture *self);
YCCPicture *ycc_load_picture(const char *path);
bool ycc_save_picture(const YCCPicture *self, const char *path);
void ycc_copy(YCCPicture *dst, const YCCPicture *src);
bool ycc_merge(YCCPicture *base, YCCPicture *add);
void ycc_delete(YCCPicture **selfp);

#endif

