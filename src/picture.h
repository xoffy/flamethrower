
#ifndef __PICTURE_H_
#define __PICTURE_H_

typedef struct {
    unsigned char   *data;
    int             width;
    int             height;
} Picture;

typedef Picture RGBAPicture;
typedef Picture YUVAPicture;

#define PIC_COMPONENTS 4
#define COLOR_CLAMP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

Picture         *picture_new(int width, int height);
Picture         *picture_clone(const Picture *src);
Picture         *picture_load(const char *path);
void            picture_delete(Picture *pic);
int             picture_save(const Picture *pic, const char *path);
void            picture_resize(Picture *pic, int new_width, int new_height);
void            picture_clear(Picture *rgba);
unsigned char   *picture_get_pixel(Picture *pic, int x, int y);
void            picture_scan(
                    Picture *pic,
                    int (*f)(
                        Picture *self,
                        unsigned char *current_pixel,
                        int x, int y,
                        void *opt_data
                    ),
                    void *data
                );
RGBAPicture     *rgba_picture_merge(
                    RGBAPicture *base,
                    RGBAPicture *added,
                    double opacity
                );

#define GET_R_FROM_YUV(yuv) ( \
    COLOR_CLAMP(0.0 \
        + (298.082 * (yuv)[0] / 256.0) \
        + (408.583 * (yuv)[2] / 256.0) \
        - 222.921) \
)

#define GET_G_FROM_YUV(yuv) ( \
    COLOR_CLAMP(0.0 \
        + (298.082 * (yuv)[0] / 256.0) \
        - (100.291 * (yuv)[1] / 256.0) \
        - (208.120 * (yuv)[2] / 256.0) \
        + 135.576) \
)

#define GET_B_FROM_YUV(yuv) ( \
    COLOR_CLAMP(0.0 \
        + (298.082 * (yuv)[0] / 256.0) \
        + (516.412 * (yuv)[1] / 256.0) \
        - 276.836) \
)

#define GET_Y_FROM_RGB(rgb) ( \
    COLOR_CLAMP(16.0 \
        + (65.738 * (rgb)[0] / 256.0) \
        + (129.057 * (rgb)[1] / 256.0) \
        + (25.064 * (rgb)[2] / 256.0)) \
)

#define GET_U_FROM_RGB(rgb) ( \
    COLOR_CLAMP(128.0 \
        - (37.945 * (rgb)[0] / 256.0) \
        - (74.494 * (rgb)[1] / 256.0) \
        + (112.439 * (rgb)[2] / 256.0)) \
)

#define GET_V_FROM_RGB(rgb) ( \
    COLOR_CLAMP(128.0 \
        + (112.439 * (rgb)[0] / 256.0) \
        - (94.154 * (rgb)[1] / 256.0) \
        - (18.285 * (rgb)[2] / 256.0)) \
)

#endif

