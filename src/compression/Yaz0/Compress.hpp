/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.

    Derived from the Orthrus project (https://github.com/NWPlayer123/Orthrus),
    ported to C++ by conhlee, 2025
*/

#ifndef YAZ0_COMPRESS_HPP
#define YAZ0_COMPRESS_HPP

#include <cstddef>

namespace Yaz0 {

namespace detail {

size_t maxCompressedSize(const size_t dataSize);
size_t compressImpl(
    const unsigned char* dataStart, const size_t dataSize,
    unsigned char* bufferStart
);

} // namespace detail

} // namespace Yaz0

#endif // YAZ0_WINDOW_HPP
