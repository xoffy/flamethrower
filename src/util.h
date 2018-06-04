
#ifndef __UTIL_H
#define __UTIL_H

#include <string>

namespace Util {
    inline unsigned char clampComponent(int v) {
        if (v > 255) {
            return 255;
        } else if (v < 0) {
            return 0;
        }
        return v;
    }

    inline std::string getFileExt(const std::string &path) {
        return path.substr(path.find_last_of(".") + 1);
    }
};

#endif
