/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.

    Ported to C++ by conhlee from original Rust source (18-07-2025)
*/

#ifndef YAZ0_WINDOW_HPP
#define YAZ0_WINDOW_HPP

#include <cstdint>

#include <cstddef>

namespace Yaz0 {

namespace detail {

class Window {
public:
    Window(const unsigned char* data, const size_t dataSize);
    ~Window() = default;

    struct SearchResult {
        uint32_t offset;
        uint32_t length;

        SearchResult() :
            offset(0), length(0)
        {}
        SearchResult(uint32_t offset, uint32_t length) :
            offset(offset), length(length)
        {}
    };

    SearchResult search(size_t searchPosition);

private:
    static constexpr size_t MIN_MATCH_LENGTH = 3;
    static constexpr size_t MAX_MATCH_LENGTH = 0xFF + 0x12;

    static constexpr size_t HASH_BITS = 15;
    static constexpr size_t HASH_SIZE = 1ull << HASH_BITS;

    static constexpr size_t HASH_MASK = HASH_SIZE - 1;
    static constexpr size_t HASH_SHIFT = (HASH_BITS + MIN_MATCH_LENGTH - 1) / MIN_MATCH_LENGTH;
    
    static constexpr size_t HASH_NULL = 0xFFFF;

    static constexpr size_t WINDOW_SIZE = 0x1000;
    static constexpr size_t WINDOW_MASK = WINDOW_SIZE - 1;

private:
    size_t updateHash(size_t hash, unsigned char byte) {
        return ((hash << HASH_SHIFT) ^ static_cast<size_t>(byte)) & HASH_MASK;
    }

    void catchUpTo(size_t newPosition);

private:
    const unsigned char* mData;
    size_t mDataSize;

    size_t mPosition;

    size_t mHashStart;
    size_t mHashEnd;

    uint16_t mHashHead[HASH_SIZE];
    uint16_t mHashTail[HASH_SIZE];
    uint16_t mHashNext[WINDOW_SIZE];
};

} // namespace detail

} // namespace Yaz0

#endif // YAZ0_WINDOW_HPP
