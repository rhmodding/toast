#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>

#include <string>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <optional>

#include "texture/TPL.hpp"

#define BYTESWAP_64 __builtin_bswap64
#define BYTESWAP_32 __builtin_bswap32
#define BYTESWAP_16 __builtin_bswap16

#define IDENTIFIER_TO_U32(char1, char2, char3, char4) ( \
    ((uint32_t)char4 << 24) | ((uint32_t)char3 << 16) | \
    ((uint32_t)char2 <<  8) | ((uint32_t)char1 <<  0) \
)

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#define IS_POWER_OF_TWO(x) ((x) > 0 && !((x) & ((x) - 1)))

#define AVERAGE_FLOATS(a, b) ((float)(a + b) / 2.f)
#define AVERAGE_INTS(a, b) ((int32_t)(a + b) / 2)
#define AVERAGE_UINTS(a, b) ((uint32_t)(a + b) / 2)

#define AVERAGE_UCHARS(a, b) ( \
    ((uint8_t)a / 2) + ((uint8_t)b / 2) + \
    (((uint8_t)a & 1) & ((uint8_t)b & 1)) \
)

#define AVERAGE_IMVEC2(vecA, vecB) ( ImVec2( (vecA.x + vecB.x) / 2.f, (vecA.y + vecB.y) / 2.f ) )

#define AVERAGE_IMVEC2_ROUND(vecA, vecB) (ImVec2( \
    (float)(int)(((vecA.x + vecB.x) / 2.f) + .5f), \
    (float)(int)(((vecA.y + vecB.y) / 2.f) + .5f) \
))

#define ROUND_IMVEC2(vec) ( ImVec2( (float)(int)(vec.x + .5f), (float)(int)(vec.y + .5f) ) )

#define GET_IMGUI_IO ImGuiIO& io = ImGui::GetIO()
#define GET_WINDOW_DRAWLIST ImDrawList* drawList = ImGui::GetWindowDrawList()

#define CENTER_NEXT_WINDOW ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f))

#define NONFATAL_ASSERT(condition, shouldReturn) \
    do { \
        if (!(condition)) { \
            std::cerr << "[NONFATAL_ASSERT] Assertion failed: (" #condition "), function " << __FUNCTION__ \
                      << ", file " << __FILE__ << ", line " << __LINE__ << "." << std::endl; \
            if (shouldReturn) return; \
        } \
    } while (false)

#define NONFATAL_ASSERT_RET(condition, ret) \
    do { \
        if (!(condition)) { \
            std::cerr << "[NONFATAL_ASSERT_RET] Assertion failed: (" #condition "), function " << __FUNCTION__ \
                      << ", file " << __FILE__ << ", line " << __LINE__ << "." << std::endl; \
            return (ret); \
        } \
    } while (false)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

namespace Common {

float byteswapFloat(float value);

bool SaveBackupFile(const char* filePath, bool once);

template <typename T>
void deleteIfNotNullptr(T*& ptr, bool setNullptr = true) {
    if (ptr)
        delete ptr;

    if (setNullptr)
        ptr = nullptr;
}

void FitRect(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale);

float EaseInOut(float t);
float EaseIn(float t);
float EaseOut(float t);

} // namespace Common

#endif // COMMON_HPP
