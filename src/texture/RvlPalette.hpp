#ifndef RVLPALETTE_HPP
#define RVLPALETTE_HPP

#include "TPL.hpp"

#include <cstdint>

#include <vector>
#include <set>

namespace RvlPalette {

[[nodiscard]] std::set<uint32_t> generate(const unsigned char* rgbaImage, unsigned pixelCount);

void readCLUT(
    std::vector<uint32_t>& colorsOut,
    const void* clutIn,

    const unsigned colorCount,

    const TPL::TPLClutFormat format
);

bool writeCLUT(
    void* clutOut,
    const std::vector<uint32_t>& colorsIn,

    const TPL::TPLClutFormat format
);

} // namespace RvlPalette

#endif // RVLPALETTE_HPP
