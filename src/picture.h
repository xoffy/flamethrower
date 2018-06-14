
#ifndef __PICTURE_H_
#define __PICTURE_H_

typedef unsigned char Byte;

struct _RGBPicture {
    Byte    *data;
    int     width;
    int     height;
    int     components;
};

struct _YCbCrPicture {
    Byte    *data;
    int     width;
    int     height;
};

typedef _RGBPicture RGBPicture;
typedef _YCbCrPicture YCbCrPicture;

#endif

