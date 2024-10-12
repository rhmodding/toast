#ifndef IMAGECONVERT_HPP
#define IMAGECONVERT_HPP

#include "TPL.hpp"

#include <vector>

namespace ImageConvert {

bool toRGBA32(
    unsigned char* buffer,
    const TPL::TPLImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data,
    const uint32_t* palette
);
bool toRGBA32(
    TPL::TPLTexture& texture,
    const unsigned char* data
);

bool fromRGBA32(
    unsigned char* buffer,
    uint32_t* paletteOut,
    uint32_t* paletteSizeOut,
    const TPL::TPLImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data
);
bool fromRGBA32(
    TPL::TPLTexture& texture,
    unsigned char* buffer
);

unsigned getImageByteSize(const TPL::TPLImageFormat type, const unsigned width, const unsigned height);
unsigned getImageByteSize(const TPL::TPLTexture& texture);

} // namespace ImageConvert

#endif // IMAGECONVERT_HPP
