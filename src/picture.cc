
#include <iostream>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

#include "picture.h"
#include "util.h"

RGBPicture::RGBPicture(int width, int height)
    : AbstractPicture(width, height), data(NULL)
{
    no_stbi = true;

    if (width > 0 && height > 0) {
        data = new Byte[width * height * 3]();
    }
}

RGBPicture::RGBPicture(const std::string &path) {
    loadFromFile(path);
}

RGBPicture::RGBPicture(const RGBPicture &rgb)
    : AbstractPicture(rgb.get_width(), rgb.get_height())
{
    data = new Byte[width * height * 3];
    std::memcpy(data, rgb.get_pixel(0, 0), width * height * 3);
}

RGBPicture::RGBPicture(const YCbCrPicture &ycbcr) {
    width = ycbcr.get_width();
    height = ycbcr.get_height();
    unsigned int size = width * height;

    no_stbi = true;
    data = new Byte[size * 3];
    unsigned int j = 0;

    for (unsigned int i = 0; i < size; i++) {
        data[j++] = Util::clampComponent(
            (298.082 * ycbcr[i].Y / 256.0)
            + (408.583 * ycbcr[i].Cr / 256.0)
            - 222.921);
        data[j++] = Util::clampComponent(
            (298.082 * ycbcr[i].Y / 256.0)
            - (100.291 * ycbcr[i].Cb / 256.0)
            - (208.120 * ycbcr[i].Cr / 256.0)
            + 135.576);
        data[j++] = Util::clampComponent(
            (298.082 * ycbcr[i].Y / 256.0)
            + (516.412 * ycbcr[i].Cb / 256.0)
            - 276.836);
    }
}

RGBPicture::~RGBPicture() {
    if (no_stbi && data) {
        // delete[] data;
    } else if (data != NULL) {
        stbi_image_free(data);
    }
}

void RGBPicture::loadFromFile(const std::string &path) {
    no_stbi = false;
    data = stbi_load(path.c_str(), &width, &height, NULL, 3);
    if (!data) {
        throw std::runtime_error("Unable to load picture.");
    }
}

void RGBPicture::resize(int desired_width, int desired_height) {
    Byte *desired_data = new Byte[desired_width * desired_height * 3];
    int rc = stbir_resize_uint8(data, width, height, 0,
        desired_data, desired_width, desired_height, 0, 3);
    if (rc == 0) {
        throw std::runtime_error("Can't resize image!");
    }

    stbi_image_free(data);
    data = desired_data;
    width = desired_width;
    height = desired_height;
}

void RGBPicture::save(const std::string &path) {
    std::string ext = Util::getFileExt(path);
    int rc = 0;

    if (ext == "png") {
        rc = stbi_write_png(path.c_str(), width, height, 3, data, 0);
    } else if (ext == "jpg" || ext == "jpeg") {
        rc = stbi_write_jpg(path.c_str(), width, height, 3, data, 90);
    } else if (ext == "bmp") {
        rc = stbi_write_bmp(path.c_str(), width, height, 3, data);
    } else {
        throw std::runtime_error("Unknown output extension!");
    }

    if (rc == 0) {
        throw std::runtime_error("Unable to save file.");
    }
}

void RGBPicture::scan(const std::function<void (int,
    int, Byte *)> &function)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            function(x, y, get_pixel(x, y));
        }
    }
}

void RGBPicture::merge(const RGBPicture &other) {
    unsigned int idx = 0;
    unsigned int size = width * height * 3;
    while (idx < size) {
        data[idx] = Util::clampComponent(data[idx] + other[idx]);
        idx++;
    }
}

// ---

void YCbCrPicture::convertFromRGB(const RGBPicture &rgb) {
    this->width = rgb.get_width();
    this->height = rgb.get_height();
    data.resize(width * height);

    for (unsigned int i = 0; i < data.size(); i++) {
        int j = i * 3;

        data[i].Y = Util::clampComponent(16.0
                + (65.7380 * rgb[j + 0] / 256.0)
                + (129.057 * rgb[j + 1] / 256.0)
                + (25.0640 * rgb[j + 2] / 256.0));
        data[i].Cb = Util::clampComponent(128.0
                - (37.9450 * rgb[j + 0] / 256.0)
                - (74.4940 * rgb[j + 1] / 256.0)
                + (112.439 * rgb[j + 2] / 256.0));
        data[i].Cr = Util::clampComponent(128.0
                + (112.439 * rgb[j + 0] / 256.0)
                - (94.1540 * rgb[j + 1] / 256.0)
                - (18.2850 * rgb[j + 2] / 256.0));
    }
}

YCbCrPicture::YCbCrPicture(int width, int height)
    : AbstractPicture(width, height)
{
    data.resize(width * height);
}

YCbCrPicture::YCbCrPicture(const RGBPicture &rgb) {
    convertFromRGB(rgb);
}

YCbCrPicture::YCbCrPicture(const std::string &path) {
    loadFromFile(path);
}

void YCbCrPicture::scan(const std::function<void (int,
    int, YCbCrPixel &)> &function)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            function(x, y, get_pixel(x, y));
        }
    }
}

void YCbCrPicture::merge(const YCbCrPicture &other, int armor) {
    if (other.width != width || other.height != height) {
        throw std::runtime_error("Attempt to merge incompatible images.");
    }

    scan([&](int x, int y, YCbCrPixel &e) {
        e.Y = Util::clampComponent(e.Y + other.get_pixel(x, y).Y - armor);
        e.Cb = Util::clampComponent(e.Cb + other.get_pixel(x, y).Cb - armor);
        e.Cr = Util::clampComponent(e.Cr + other.get_pixel(x, y).Cr - armor);
    });
}

void YCbCrPicture::loadFromFile(const std::string &path) {
    RGBPicture bridge(path);
    convertFromRGB(bridge);
}

void YCbCrPicture::resize(int desired_width, int desired_height) {
    RGBPicture bridge(*this);
    bridge.resize(desired_width, desired_height);
    convertFromRGB(bridge);
}

void YCbCrPicture::save(const std::string &path) {
    RGBPicture bridge(*this);
    bridge.save(path);
}

// ---
#if 0
Picture::Picture(int w, int h) : width(w), height(h) {
    if (width > 0 && height > 0) {
        data.resize(width * height, Pixel(0, 0, 0));
    }
}

Picture::Picture(const std::string &path,
    int desired_width, int desired_height)
{
    int components;
    unsigned char *rgb_data = stbi_load(path.c_str(),
        &width, &height, &components, 0);
    if (!rgb_data) {
        throw std::runtime_error("Unable to load picture.");
    }

    if (desired_width > 0 && desired_height > 0) {
        rgb_data = Picture::resize(rgb_data, width, height, desired_width, desired_height);
        width = desired_width;
        height = desired_height;
    }

    data = Picture::get_YCbCr(rgb_data, width, height);
    stbi_image_free(rgb_data);
}

unsigned char *Picture::get_raw_rgb() {
    unsigned char *rgb_data = new unsigned char[width * height * 3];

    for (unsigned int i = 0; i < data.size(); i++) {
        int j = i * 3;

        rgb_data[j + 0] = Util::clampComponent(
            (298.082 * data[i].Y / 256.0)
            + (408.583 * data[i].Cr / 256.0)
            - 222.921);
        rgb_data[j + 1] = Util::clampComponent(
            (298.082 * data[i].Y / 256.0)
            - (100.291 * data[i].Cb / 256.0)
            - (208.120 * data[i].Cr / 256.0)
            + 135.576);
        rgb_data[j + 2] = Util::clampComponent(
            (298.082 * data[i].Y / 256.0)
            + (516.412 * data[i].Cb / 256.0)
            - 276.836);
    }

    return rgb_data;
}

std::vector<Pixel> Picture::get_YCbCr(unsigned char *rgb_data,
    int width, int height)
{
    std::vector<Pixel> data;

    data.resize(width * height);
    for (unsigned int i = 0; i < data.size(); i++) {
        int j = i * 3;

        data[i].Y = Util::clampComponent(16.0
                + (65.7380 * rgb_data[j + 0] / 256.0)
                + (129.057 * rgb_data[j + 1] / 256.0)
                + (25.0640 * rgb_data[j + 2] / 256.0));
        data[i].Cb = Util::clampComponent(128.0
                - (37.9450 * rgb_data[j + 0] / 256.0)
                - (74.4940 * rgb_data[j + 1] / 256.0)
                + (112.439 * rgb_data[j + 2] / 256.0));
        data[i].Cr = Util::clampComponent(128.0
                + (112.439 * rgb_data[j + 0] / 256.0)
                - (94.1540 * rgb_data[j + 1] / 256.0)
                - (18.2850 * rgb_data[j + 2] / 256.0));
    }

    return data;
}

void Picture::save(const std::string &path) {
    unsigned char *rgb_data = get_raw_rgb();
    std::string ext = Util::getFileExt(path);
    int rc = 0;

    if (ext == "png") {
        rc = stbi_write_png(path.c_str(), width, height, 3, rgb_data, 0);
    } else if (ext == "jpg" || ext == "jpeg") {
        rc = stbi_write_jpg(path.c_str(), width, height, 3, rgb_data, 100);
    } else if (ext == "bmp") {
        rc = stbi_write_bmp(path.c_str(), width, height, 3, rgb_data);
    } else {
        throw std::runtime_error("Unknown output extension!");
    }

    if (rc == 0) {
        throw std::runtime_error("Unable to save file.");
    }
}

void Picture::scan(const std::function<void (int, int, Pixel &)> &f) {
    for (int cy = 0; cy < height; cy++) {
        for (int cx = 0; cx < width; cx++) {
            f(cx, cy, get_pixel(cx, cy));
        }
    }
}

Pixel &Picture::get_pixel(int x, int y) {
    // return data + (width * components * y + components * x);
    return data[width * y + x];
}

unsigned char *Picture::resize(unsigned char *rgb_data, int w, int h, int nw, int nh) {
    unsigned char *new_rgb_data = new unsigned char[nw * nh * 3];
    int rc = stbir_resize_uint8(rgb_data, w, h, 0, new_rgb_data, nw, nh, 0, 3);

    if (rc == 0) {
        throw std::runtime_error("Can't resize image!");
    }

    stbi_image_free(rgb_data);
    return new_rgb_data;
}

void Picture::resize(int nw, int nh) {
    unsigned char *rgb_data = Picture::resize(Picture::get_raw_rgb(),
        width, height, nw, nh);
    data = get_YCbCr(rgb_data, nw, nh);
}

void Picture::merge(Picture &other) {
    if (other.width != width || other.height != height) {
        throw std::runtime_error("Attempt to merge incompatible images.");
    }

    scan([&](int x, int y, Pixel &px) {
        Pixel &opx = other.get_pixel(x, y);
        px.Y = Util::clampComponent(px.Y + opx.Y);
        px.Cb = Util::clampComponent(px.Cb + opx.Cb);
        px.Cr = Util::clampComponent(px.Cr + opx.Cr);
    });
}
#endif