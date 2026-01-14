#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint> // IWYU pragma: keep

#include <bit> // IWYU pragma: keep

#include <imgui.h>

#include "Logging.hpp" // IWYU pragma: keep

#define BYTESWAP_64 __builtin_bswap64
#define BYTESWAP_32 __builtin_bswap32
#define BYTESWAP_16 __builtin_bswap16

#define BYTESWAP_FLOAT(x) ( \
    std::bit_cast<float, uint32_t>( \
        BYTESWAP_32(std::bit_cast<uint32_t, float>((x))) \
    ) \
)

#define IDENTIFIER_TO_U32(char1, char2, char3, char4) ( \
    ((uint32_t)(char4) << 24) | ((uint32_t)(char3) << 16) | \
    ((uint32_t)(char2) <<  8) | ((uint32_t)(char1) <<  0) \
)

#define ARRAY_LENGTH(x) ( sizeof((x)) / sizeof(((x))[0]) )

#define IS_POWER_OF_TWO(x) ( ((x) > 0) && !((x) & ((x) - 1)) )

#define ALIGN_DOWN_2(value) ( (value) & ~(2 - 1) )
#define ALIGN_UP_2(value)   ( ((value) + 2 - 1) & ~(2 - 1) )

#define ALIGN_DOWN_4(value) ( (value) & ~(4 - 1) )
#define ALIGN_UP_4(value)   ( ((value) + 4 - 1) & ~(4 - 1) )

#define ALIGN_DOWN_16(value) ( (value) & ~(16 - 1) )
#define ALIGN_UP_16(value)   ( ((value) + 16 - 1) & ~(16 - 1) )

#define ALIGN_DOWN_32(value) ( (value) & ~(32 - 1) )
#define ALIGN_UP_32(value)   ( ((value) + 32 - 1) & ~(32 - 1) )

#define ALIGN_DOWN_64(value) ( (value) & ~(64 - 1) )
#define ALIGN_UP_64(value)   ( ((value) + 64 - 1) & ~(64 - 1) )

#define ALIGN_DOWN_128(value) ( (value) & ~(128 - 1) )
#define ALIGN_UP_128(value)   ( ((value) + 128 - 1) & ~(128 - 1) )

// Get length of string literal. No pointers please!
#define STR_LIT_LEN(stringLiteral) ( sizeof((stringLiteral)) - 1 )

// t is a normalized float ([0 .. 1])
#define LERP_INTS(a, b, t) ( (int)((int)(a) + (int)((float)(t) * ((int)(b) - (int)(a)))) )

#define AVERAGE_FLOATS(a, b) ( (((float)(a) + (float)(b))) / 2.f )
#define AVERAGE_INTS(a, b)   ( (((int)(a) + (int)(b))) / 2 )
#define AVERAGE_UINTS(a, b)  ( (((unsigned int)(a) + (unsigned int)(b))) / 2 )

#define AVERAGE_UCHARS(a, b) ( \
    ((unsigned char)(a) / 2) + ((unsigned char)(b) / 2) + \
    (((unsigned char)(a) & 1) & ((unsigned char)(b) & 1)) \
)

#define AVERAGE_IMVEC2(vecA, vecB) (ImVec2( \
    ((vecA).x + (vecB).x) / 2.f, ((vecA).y + (vecB).y) / 2.f \
))
#define AVERAGE_IMVEC2_ROUND(vecA, vecB) (ImVec2( \
    (float)(int)(((vecA).x + (vecB).x) / 2.f + .5f), \
    (float)(int)(((vecA).y + (vecB).y) / 2.f + .5f) \
))

#define ROUND_FLOAT(value) ( (float)(int)((float)(value) + .5f) )
#define FLOOR_FLOAT(value) ( (float)(int)((float)(value)) )
#define CEIL_FLOAT(value)  ( (float)(int)((float)(value)) + (((float)(value) > (int)(float)(value)) ? 1.f : 0.f) )

#define ROUND_IMVEC2(vec) (ImVec2( \
    ROUND_FLOAT((vec).x), ROUND_FLOAT((vec).y) \
))

#if !defined(NDEBUG)

#define NONFATAL_ASSERT_RET(condition, shouldReturn) \
    do { \
        if (!(condition)) { \
            Logging::error( \
                "[NONFATAL_ASSERT_RET] Assertion failed: (" #condition "), function {}, file {}, line {}.", \
                __FUNCTION__, __FILE__, __LINE__ \
            ); \
            if (shouldReturn) return; \
        } \
    } while (0)

#define NONFATAL_ASSERT_RETVAL(condition, ret) \
    do { \
        if (!(condition)) { \
            Logging::error( \
                "[NONFATAL_ASSERT_RETVAL] Assertion failed: (" #condition "), function {}, file {}, line {}.", \
                __FUNCTION__, __FILE__, __LINE__ \
            ); \
            return (ret); \
        } \
    } while (0)

#else // !defined(NDEBUG)

#define NONFATAL_ASSERT_RET(condition, shouldReturn) ((void)0)
#define NONFATAL_ASSERT_RETVAL(condition, shouldReturn) ((void)0)

#endif // !defined(NDEBUG)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif // COMMON_HPP
