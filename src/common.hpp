#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

#include <iostream>

#include <bit>

#include <imgui.h>

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

#define ALIGN_DOWN_4(value) ( (value) & ~(4 - 1) )
#define ALIGN_UP_4(value)   ( ((value) + 4 - 1) & ~(4 - 1) )

#define ALIGN_DOWN_16(value) ( (value) & ~(16 - 1) )
#define ALIGN_UP_16(value)   ( ((value) + 16 - 1) & ~(16 - 1) )

#define ALIGN_DOWN_32(value) ( (value) & ~(32 - 1) )
#define ALIGN_UP_32(value)   ( ((value) + 32 - 1) & ~(32 - 1) )

#define ALIGN_DOWN_64(value) ( (value) & ~(64 - 1) )
#define ALIGN_UP_64(value)   ( ((value) + 64 - 1) & ~(64 - 1) )

#define STR_LIT_LEN(stringLiteral) ( sizeof((stringLiteral)) - 1 )

#define AVERAGE_FLOATS(a, b) ( ((float)((a) + (b))) / 2.f )
#define AVERAGE_INTS(a, b)   ( ((int)((a) + (b))) / 2 )
#define AVERAGE_UINTS(a, b)  ( ((unsigned int)((a) + (b))) / 2 )

#define AVERAGE_UCHARS(a, b) ( \
    ((uint8_t)(a) / 2) + ((uint8_t)(b) / 2) + \
    (((uint8_t)(a) & 1) & ((uint8_t)(b) & 1)) \
)

#define AVERAGE_IMVEC2(vecA, vecB) (ImVec2( \
    ((vecA).x + (vecB).x) / 2.f, ((vecA).y + (vecB).y) / 2.f \
))
#define AVERAGE_IMVEC2_ROUND(vecA, vecB) (ImVec2( \
    (float)(int)(((vecA).x + (vecB).x) / 2.f + .5f), \
    (float)(int)(((vecA).y + (vecB).y) / 2.f + .5f) \
))

#define ROUND_FLOAT(value) ( (float)(int)((value) + .5f) )
#define FLOOR_FLOAT(value) ( (float)(int)((value)) )

#define ROUND_IMVEC2(vec) (ImVec2( \
    ROUND_FLOAT((vec).x), ROUND_FLOAT((vec).y) \
))

#define CENTER_NEXT_WINDOW() \
    do { \
        ImGui::SetNextWindowPos( \
            ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f) \
        ); \
    } while (0)

#ifndef NDEBUG

#define NONFATAL_ASSERT_RET(condition, shouldReturn) \
    do { \
        if (!(condition)) { \
            std::cerr << "[NONFATAL_ASSERT_RET] Assertion failed: (" #condition "), " \
                      << "function " << __FUNCTION__ << ", file " << __FILE__ \
                      << ", line " << __LINE__ << "." << std::endl; \
            if (shouldReturn) return; \
        } \
    } while (0)

#define NONFATAL_ASSERT_RETVAL(condition, ret) \
    do { \
        if (!(condition)) { \
            std::cerr << "[NONFATAL_ASSERT_RETVAL] Assertion failed: (" #condition "), " \
                      << "function " << __FUNCTION__ << ", file " << __FILE__ \
                      << ", line " << __LINE__ << "." << std::endl; \
            return (ret); \
        } \
    } while (0)

#else // NDEBUG

#define NONFATAL_ASSERT_RET(condition, shouldReturn) ((void)0)
#define NONFATAL_ASSERT_RETVAL(condition, shouldReturn) ((void)0)

#endif // NDEBUG

#define TRAP() \
    do { \
        fprintf(stderr, "TRAP at %s:%d\n", __FILE__, __LINE__); \
        fflush(stderr); \
        __builtin_trap(); \
        __builtin_unreachable(); \
    } while (0)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif // COMMON_HPP
