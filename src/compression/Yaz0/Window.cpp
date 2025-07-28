/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.

    Ported to C++ by conhlee from original Rust source (18-07-2025)
*/

#include "Window.hpp"

#include <cstring>

#include <algorithm>

#include <stdexcept>

namespace Yaz0 {

namespace detail {

Window::Window(const unsigned char* data, const size_t dataSize) :
    mData(data), mDataSize(dataSize),
    mPosition(0)
{
    size_t hash = 0;
    for (size_t i = 0; i < std::min(MIN_MATCH_LENGTH - 1, dataSize); i++) {
        hash = updateHash(hash, data[i]);
    }

    mHashStart = hash;
    mHashEnd = hash;

    std::fill(mHashHead, mHashHead + HASH_SIZE, HASH_NULL);
    std::fill(mHashTail, mHashTail + HASH_SIZE, HASH_NULL);
    std::fill(mHashNext, mHashNext + WINDOW_SIZE, HASH_NULL);
}

static inline size_t commonPrefixLength(
    const unsigned char* a, size_t aLen,
    const unsigned char* b, size_t bLen,
    size_t maxLen
) {
    size_t limit = aLen < bLen ? aLen : bLen;
    if (maxLen < limit) {
        limit = maxLen;
    }

    for (size_t i = 0; i < limit; i++) {
        if (a[i] != b[i]) {
            return i;
        }
    }
    return limit;
}

Window::SearchResult Window::search(size_t searchPosition) {
#if !defined(NDEBUG)
    if (searchPosition < mPosition) {
        throw std::runtime_error("Yaz0::Window::search: moved backwards!");
    }

    if (searchPosition >= mDataSize) {
        throw std::runtime_error("Yaz0::Window::search: search position is out of bounds!");
    }
#endif // !defined(NDEBUG)

    size_t maxMatch = std::min(mDataSize - searchPosition, MAX_MATCH_LENGTH);
    if (maxMatch < MIN_MATCH_LENGTH) {
        return Window::SearchResult();
    }

    catchUpTo(searchPosition);

    size_t hash = updateHash(mHashEnd, mData[mPosition + MIN_MATCH_LENGTH - 1]);

    size_t position = mHashHead[hash];
    size_t bestLength = MIN_MATCH_LENGTH - 1;
    size_t bestOffset = 0;

    while (position != HASH_NULL) {
        size_t matchOffset = searchPosition - 1 - ((searchPosition - (position + 1)) & WINDOW_MASK);
    
        if (
            (mData[searchPosition + 0] == mData[matchOffset + 0]) &&
            (mData[searchPosition + 1] == mData[matchOffset + 1]) &&
            (mData[searchPosition + bestLength] == mData[matchOffset + bestLength])
        ) {
            size_t candidateLength = MIN_MATCH_LENGTH + commonPrefixLength(
                mData + searchPosition + MIN_MATCH_LENGTH, mDataSize - (searchPosition + MIN_MATCH_LENGTH),
                mData + matchOffset + MIN_MATCH_LENGTH, mDataSize - (matchOffset + MIN_MATCH_LENGTH),
                MAX_MATCH_LENGTH - MIN_MATCH_LENGTH
            );

            if (candidateLength > bestLength) {
                bestLength = candidateLength;
                bestOffset = matchOffset;

                if (bestLength == maxMatch)
                    break;
            }
        }

        position = mHashNext[position];
    }

    return SearchResult(
        static_cast<uint32_t>(bestOffset), static_cast<uint32_t>(bestLength)
    );
}

void Window::catchUpTo(size_t newPosition) {
#if !defined(NDEBUG)
    newPosition = std::min(newPosition, mDataSize);
#endif // !defined(NDEBUG)

    while (mPosition < newPosition) {
        size_t pos = mPosition & WINDOW_MASK;

        if (mPosition >= WINDOW_SIZE) {
            if ((mPosition - WINDOW_SIZE + MIN_MATCH_LENGTH - 1) < mDataSize) {
                mHashStart = updateHash(
                    mHashStart,
                    mData[mPosition - WINDOW_SIZE + MIN_MATCH_LENGTH - 1]
                );

                uint16_t head = mHashHead[mHashStart];
                if (head != HASH_NULL) {
                    uint16_t next = mHashNext[head];
                    mHashHead[mHashStart] = next;
                    if (next == HASH_NULL) {
                        mHashTail[mHashStart] = HASH_NULL;
                    }
                }
            }
        }

        if ((mPosition + MIN_MATCH_LENGTH - 1) < mDataSize) {
            mHashEnd = updateHash(mHashEnd, mData[mPosition + MIN_MATCH_LENGTH - 1]);

            uint16_t tail = mHashTail[mHashEnd];

            mHashNext[pos] = HASH_NULL;
            mHashTail[mHashEnd] = static_cast<uint16_t>(pos);

            if (tail == HASH_NULL) {
                mHashHead[mHashEnd] = static_cast<uint16_t>(pos);
            }
            else {
                mHashNext[tail] = static_cast<uint16_t>(pos);
            }
        }

        mPosition++;
    }
}

} // namespace detail

} // namespace Yaz0
