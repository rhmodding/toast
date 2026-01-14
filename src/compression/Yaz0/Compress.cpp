/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.

    Derived from the Orthrus project (https://github.com/NWPlayer123/Orthrus),
    ported to C++ by conhlee, 2025
*/

#include "Compress.hpp"

#include "Window.hpp"

namespace Yaz0 {

namespace detail {

constexpr size_t HEADER_SIZE = 0x10;

constexpr size_t MIN_MATCH_LENGTH = 3;
constexpr size_t MAX_MATCH_LENGTH = 0xFF + 0x12;

size_t maxCompressedSize(const size_t dataSize) {
    // Header + every byte + all op bytes needed (just copy, no RLE).
    return HEADER_SIZE + dataSize + ((dataSize + 7) / 8);
}

size_t compressImpl(
    const unsigned char* dataStart, const size_t dataSize,
    unsigned char* bufferStart
) {
    Window* window = new Window(dataStart, dataSize);

    size_t srcPosition = 0;
    size_t dstPosition = HEADER_SIZE + 1;

    size_t opPosition = HEADER_SIZE;
    size_t opMask = (1 << 7);

    while (srcPosition < dataSize) {
        Window::SearchResult search = window->search(srcPosition);
        if (search.length < MIN_MATCH_LENGTH) {
            bufferStart[opPosition] |= opMask;
            bufferStart[dstPosition++] = dataStart[srcPosition++];
        }
        else {
            Window::SearchResult secondSearch = window->search(srcPosition + 1);
            if ((search.length + 1) < secondSearch.length) {
                bufferStart[opPosition] |= opMask;
                bufferStart[dstPosition++] = dataStart[srcPosition++];

                opMask >>= 1;
                if (opMask == 0) {
                    opMask = (1 << 7);
                    opPosition = dstPosition++;
                    bufferStart[opPosition] = 0x00;
                }

                search = secondSearch;
            }
            
            size_t lookbackOffset = srcPosition - search.offset - 1;

            if (search.length >= 0x12) {
                bufferStart[dstPosition++] = (lookbackOffset >> 8) & 0xFF;
                bufferStart[dstPosition++] = (lookbackOffset >> 0) & 0xFF;
                bufferStart[dstPosition++] = (search.length - 0x12) & 0xFF;
            }
            else {
                bufferStart[dstPosition++] = (((search.length - 2) << 4) | (lookbackOffset >> 8)) & 0xFF;
                bufferStart[dstPosition++] = (lookbackOffset >> 0) & 0xFF;
            }

            srcPosition += search.length;
        }

        opMask >>= 1;
        if (opMask == 0) {
            opMask = (1 << 7);
            opPosition = dstPosition++;
            bufferStart[opPosition] = 0x00;
        }
    }

    delete window;

    return dstPosition;
}

} // namespace detail

} // namespace Yaz0
