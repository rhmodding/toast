#ifndef RVL_IMAGECONVERT_HPP
#define RVL_IMAGECONVERT_HPP

#include <cstdint>

#include "TPL.hpp"

namespace RvlImageConvert {

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

// Note: for paletted image types this does not include the lookup table.
unsigned getImageByteSize(const TPL::TPLImageFormat type, const unsigned width, const unsigned height);
// Note: for paletted image types this does not include the lookup table.
unsigned getImageByteSize(const TPL::TPLTexture& texture);

} // namespace RvlImageConvert

#endif // RVL_IMAGECONVERT_HPP
