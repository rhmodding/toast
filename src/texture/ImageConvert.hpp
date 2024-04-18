#ifndef IMAGECONVERT_HPP
#define IMAGECONVERT_HPP

#include "./TPL.hpp"

#include <vector>

namespace ImageConvert {
    bool toRGBA32(std::vector<char>& buffer, const TPL::TPLImageFormat type, uint16_t srcWidth, uint16_t srcHeight, const std::vector<char>& data);

    size_t getImageByteSize(const TPL::TPLImageFormat type, uint16_t width, uint16_t height);
}

#endif // IMAGECONVERT_HPP