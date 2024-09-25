#ifndef IMAGECONVERT_HPP
#define IMAGECONVERT_HPP

#include "TPL.hpp"

#include <vector>

namespace ImageConvert {

bool toRGBA32(
    unsigned char* buffer,
    const TPL::TPLImageFormat type,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data,
    uint32_t* colorPalette = nullptr
);
bool fromRGBA32(
    unsigned char* buffer,
    const TPL::TPLImageFormat type,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data,
    std::vector<uint32_t>* colorPalette = nullptr
);

unsigned getImageByteSize(const TPL::TPLImageFormat type, const unsigned width, const unsigned height);
unsigned getImageByteSize(const TPL::TPLTexture& texture);

} // namespace ImageConvert

#endif // IMAGECONVERT_HPP
