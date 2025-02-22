#ifndef CTR_IMAGECONVERT_HPP
#define CTR_IMAGECONVERT_HPP

#include <cstdint>

#include "CTPK.hpp"

namespace CtrImageConvert {

bool toRGBA32(
    unsigned char* buffer,
    const CTPK::CTPKImageFormat format,
    const unsigned srcWidth,
    const unsigned srcHeight,
    const unsigned char* data
);
bool toRGBA32(
    CTPK::CTPKTexture& texture,
    const unsigned char* data
);

unsigned getImageByteSize(const CTPK::CTPKImageFormat type, const unsigned width, const unsigned height);
unsigned getImageByteSize(const CTPK::CTPKTexture& texture);

} // namespace CtrImageConvert

#endif // CTR_IMAGECONVERT_HPP
