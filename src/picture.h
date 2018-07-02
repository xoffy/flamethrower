
#ifndef __PICTURE_H_
#define __PICTURE_H_

typedef struct {
    unsigned char   *data;
    int             width;
    int             height;
} RGBAPicture;

#define RGBA_COMPONENTS 4
#define COLOR_CLAMP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : 0))

unsigned char clamp_comp(int comp);

RGBAPicture *picture_new(int width, int height);
RGBAPicture *picture_load(const char *path);
void picture_delete(RGBAPicture *rgb);

int picture_save(const RGBAPicture *rgb, const char *path);
void picture_resize(RGBAPicture *rgb, int new_width, int new_height);

#endif

