#ifndef IMAGECONVERT_HPP
#define IMAGECONVERT_HPP

#include "TPL.hpp"

#include <vector>

namespace ImageConvert {
    bool toRGBA32(
        std::vector<unsigned char>& buffer,
        const TPL::TPLImageFormat type,
        const uint16_t srcWidth,
        const uint16_t srcHeight,
        const unsigned char* data,
        uint32_t* colorPalette = nullptr
    );
    bool fromRGBA32(
        std::vector<unsigned char>& buffer,
        const TPL::TPLImageFormat type,
        const uint16_t srcWidth,
        const uint16_t srcHeight,
        const unsigned char* data,
        std::vector<uint32_t>* colorPalette = nullptr
    );

    uint32_t getImageByteSize(const TPL::TPLImageFormat type, const uint16_t width, const uint16_t height);
    uint32_t getImageByteSize(const TPL::TPLTexture& texture);
}

#endif // IMAGECONVERT_HPP