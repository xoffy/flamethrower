
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

#include "picture.h"

Picture::Picture(const std::string &path) {
    int components;
    unsigned char *rgb_data = stbi_load(path.c_str(),
        &width, &height, &components, 0);
    if (!rgb_data) {
        throw std::runtime_error("Unable to load picture.");
    }

    data.resize(width * height);

    for (unsigned int i = 0; i < data.size(); i++) {
        int j = i * components;

        data[i].setY(16
                + (65.7380 * rgb_data[j + 0] / 256)
                + (129.057 * rgb_data[j + 1] / 256)
                + (25.0640 * rgb_data[j + 2] / 256));
        data[i].setCb(128
                - (37.9450 * rgb_data[j + 0] / 256)
                - (74.4940 * rgb_data[j + 1] / 256)
                + (112.439 * rgb_data[j + 2] / 256));
        data[i].setCr(128
                + (112.439 * rgb_data[j + 0] / 256)
                - (94.1540 * rgb_data[j + 1] / 256)
                - (18.2850 * rgb_data[j + 2] / 256));
    }

    stbi_image_free(rgb_data);
}

void Picture::save(const std::string &path) {
    unsigned char *rgb_data = new unsigned char[width * height * 3];

    for (unsigned int i = 0; i < data.size(); i++) {
        int j = i * 3;

        rgb_data[j + 0] = clampComponent((298.082 * data[i].getY() / 256)
                + (408.583 * data[i].getCr() / 256)
                - 222.921);
        rgb_data[j + 1] = clampComponent((298.082 * data[i].getY() / 256)
                - (100.291 * data[i].getCb() / 256)
                - (208.120 * data[i].getCr() / 256)
                + 135.576);
        rgb_data[j + 2] = clampComponent((298.082 * data[i].getY() / 256)
                + (516.412 * data[i].getCb() / 256)
                - 276.836);
    }

    auto ext = path.substr(path.find_last_of(".") + 1);
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

void Picture::resize(int newWidth, int newHeight) {
#if 0
    unsigned char *newData =
        new unsigned char[newWidth * newHeight * components];
    int rc = stbir_resize_uint8(data, width, height, 0,
        newData, newWidth, newHeight, 0, components);

    if (rc == 0) {
        throw runtime_error("Can't resize image!");
    }

    stbi_image_free(data);

    data = newData;
    width = newWidth;
    height = newHeight;
#endif
}
