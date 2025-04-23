#ifndef APNGHACK_HPP
#define APNGHACK_HPP

#include <vector>

namespace APNGHack {

struct BuildFrame {
    std::vector<unsigned char> rgbaData; // Expected to be (width * height * 4) bytes in size.
    unsigned width, height; // Must not be larger than the canvas size.
    unsigned offsetX { 0 }, offsetY { 0 }; // Originates from top-left.

    unsigned holdFrames { 30 };
};

struct BuildData {
    std::vector<BuildFrame> frames;
    unsigned canvasW, canvasH;
    unsigned frameRate { 60 };
};

[[nodiscard]] std::vector<unsigned char> build(const BuildData& buildData);

} // namespace APNGHack

#endif // APNGHACK_HPP
