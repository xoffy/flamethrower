#ifndef PICTURE_H
#define PICTURE_H

#include <string>
#include <vector>
#include <functional>

#if 0

class Picture
{
public:
    Picture(int width = -1, int height = -1);
    explicit Picture(const std::string &path,
        int desired_width = -1, int desired_height = -1);

    void save(const std::string &path);

    void scan(const std::function<void (int, int, Pixel &)> &f);

    void resize(int nw, int nh);
    static unsigned char *resize(unsigned char *rgb_data, int w, int h, int nw, int nh);
    void merge(Picture &other);

    Pixel &get_pixel(int x, int y);
    int get_width() const { return width; }
    int get_height() const { return height; }

    unsigned char *get_raw_rgb();
    static std::vector<Pixel> get_YCbCr(unsigned char *rgb_data, int width, int height);

private:
    std::vector<Pixel> data;

    int width, height;
};

#endif

class AbstractPicture {
public:
    int get_width() const { return width; }
    int get_height() const { return height; }

protected:
    int width, height;

    AbstractPicture(int width = 0, int height = 0)
        : width(width), height(height) { }
};

typedef unsigned char Byte;
class YCbCrPicture;

class RGBPicture : public AbstractPicture {
public:
    RGBPicture(int width, int height);
    RGBPicture(const RGBPicture &rgb);
    RGBPicture(const YCbCrPicture &ycbcr);
    explicit RGBPicture(const std::string &path);
    ~RGBPicture();
    void loadFromFile(const std::string &path);
    void save(const std::string &path);
    void resize(int desired_width, int desired_height);
    void scan(const std::function<void (int, int, Byte *)> &f);
    void merge(const RGBPicture &other);

    Byte operator[](unsigned int idx) const {
        return data[idx];
    }

    Byte *get_pixel(int x, int y) const {
        return data + (y * width * 3 + x * 3);
    }

private:
    Byte *data;
    bool no_stbi = false;
};

struct YCbCrPixel {
    Byte Y, Cb, Cr;

    YCbCrPixel(Byte y = 128, Byte cb = 128, Byte cr = 128)
        : Y(y), Cb(cb), Cr(cr) { }
};

class YCbCrPicture : public AbstractPicture {
public:
    YCbCrPicture(int width, int height);
    YCbCrPicture(const RGBPicture &rgb);
    explicit YCbCrPicture(const std::string &path);
    void scan(const std::function<void (int, int, YCbCrPixel &)> &f);
    void merge(const YCbCrPicture &other, int armor = 0);

    // "bridged" methods:
    //

    void loadFromFile(const std::string &path);
    void save(const std::string &path);
    void resize(int desired_width, int desired_height);

    YCbCrPixel &operator[](unsigned int idx) {
        return data[idx];
    }

    const YCbCrPixel &operator[](unsigned int idx) const {
        return data[idx];
    }

    YCbCrPixel &get_pixel(int x, int y) {
        return data[y * width + x];
    }

    const YCbCrPixel &get_pixel(int x, int y) const {
        return data[y * width + x];
    }

    RGBPicture toRGB() const {
        return RGBPicture(*this);
    }

private:
    std::vector<YCbCrPixel> data;

    void convertFromRGB(const RGBPicture &rgb);
};

#endif // PICTURE_H
