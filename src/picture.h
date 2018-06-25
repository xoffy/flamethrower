
#ifndef __PICTURE_H_
#define __PICTURE_H_

struct _RGBPicture {
    unsigned char   *data;
    int             width;
    int             height;
    int             components;
};

struct _YCbCrPicture {
    unsigned char   *data;
    int             width;
    int             height;
    int             components;
};

typedef struct _RGBPicture RGBPicture;
typedef struct _YCbCrPicture YCbCrPicture;

unsigned char clamp_comp(int comp);

RGBPicture *rgb_picture_new(void);
RGBPicture *rgb_picture_new_dummy(int width, int height, int components);
RGBPicture *rgb_picture_load(const char *path);
RGBPicture *rgb_picture_from_ycbcr(const YCbCrPicture *ycbcr);
void rgb_picture_delete(RGBPicture *rgb);
int rgb_picture_write(const RGBPicture *rgb, const char *path);
void rgb_picture_resize(RGBPicture *rgb, int nw, int nh);

YCbCrPicture *ycbcr_picture_new(void);
YCbCrPicture *ycbcr_picture_dummy(int width, int height);
YCbCrPicture *ycbcr_picture_from_rgb(const RGBPicture *rgb);
void ycbcr_picture_delete(YCbCrPicture *ycbcr);
unsigned char *ycbcr_picture_get_pixel(const YCbCrPicture *ycbcr, int x, int y);
void ycbcr_picture_scan(YCbCrPicture *ycbcr,
    int (*func)(YCbCrPicture *, int x, int y, unsigned char *));
void ycbcr_picture_merge(YCbCrPicture *ycbcr, YCbCrPicture *add);
YCbCrPicture *ycbcr_picture_copy(YCbCrPicture *orig);

YCbCrPicture *ycbcr_picture_brdg_load(const char *path);
void ycbcr_picture_brdg_write(YCbCrPicture *ycbcr, const char *path);
void ycbcr_picture_brdg_resize(YCbCrPicture **ycbcr, int nw, int nh);
void ycbcr_picture_reset(YCbCrPicture *ycbcr);

#define YCBCR_COMPONENTS    3


#endif

