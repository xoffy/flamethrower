#ifndef PICTURE_H
#define PICTURE_H

#include <string>
#include <vector>
#include <functional>

inline unsigned char clampComponent(int v) {
    if (v > 255) {
        return 255;
    } else if (v < 0) {
        return 0;
    }
    return v;
}

class Pixel {
public:
    unsigned char getY() { return Y; }
    unsigned char getCb() { return Cb; }
    unsigned char getCr() { return Cr; }

    void setY(int _Y) { Y = clampComponent(_Y); }
    void setCb(int _Cb) { Cb = clampComponent(_Cb); }
    void setCr(int _Cr) { Cr = clampComponent(_Cr); }

    void addY(int _Y) { Y = clampComponent(Y + _Y); }
    void addCb(int _Cb) { Cb = clampComponent(Cb + _Cb); }
    void addCr(int _Cr) { Cr = clampComponent(Cr + _Cr); }

private:
    unsigned char Y;
    unsigned char Cb;
    unsigned char Cr;
};

class Picture
{
public:
    explicit Picture(const std::string &path);

    void save(const std::string &path);

    void scan(const std::function<void (int, int, Pixel &)> &f);

    void resize(int w, int h);

    Pixel &get_pixel(int x, int y);
    int get_width() { return width; }
    int get_height() { return height; }

private:
    std::vector<Pixel> data;

    int width, height;
};

#endif // PICTURE_H
