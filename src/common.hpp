#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>

#undef NDEBUG
#include <assert.h>

#include <GLFW/glfw3.h>

#include "imgui.h"

#include <optional>

#include "texture/TPL.hpp"

//#include "stb/stb_image_write.h"

#ifdef _MSC_VER
    #define BYTESWAP_64 _byteswap_uint64
    #define BYTESWAP_32 _byteswap_ulong
    #define BYTESWAP_16 _byteswap_ushort
#else
    #define BYTESWAP_64 __builtin_bswap64
    #define BYTESWAP_32 __builtin_bswap32
    #define BYTESWAP_16 __builtin_bswap16
#endif

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#define GET_IMGUI_IO ImGuiIO& io = ImGui::GetIO()
#define GET_WINDOW_DRAWLIST ImDrawList* drawList = ImGui::GetWindowDrawList()

namespace Common {
    // Byte-swap and cast uint8_t[4] to LE float
    float readBigEndianFloat(const uint8_t* bytes);

    // Cast LE float to uint8_t[4] and byte-swap
    void writeBigEndianFloat(float value, uint8_t* bytes);
    
    template <typename T>
    void deleteIfNotNullptr(T*& ptr, bool setNullptr = true) {
        if (ptr)
            delete ptr;

        if (setNullptr)
            ptr = nullptr;
    }

    std::string randomString(const uint32_t length);

    void fitRectangle(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale);

    float EaseInOut(float t);
    float EaseIn(float t);
    float EaseOut(float t);

    constexpr ImVec4 RGBAtoImVec4(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }

    // Simple helper function to load an image into a OpenGL texture with common settings
    bool LoadTextureFromFile(const char* filename, GLuint* texturePtr, int* width, int* height);
    bool LoadTextureFromMem(const unsigned char* buffer, const uint32_t size, GLuint* texturePtr, int* width, int* height);

    struct Image {
        int width{ 0 };
        int height{ 0 };

        GLuint texture{ 0 };

        // Used when converting to a TPL texture.
        TPL::TPLImageFormat tplOutFormat{ TPL::TPL_IMAGE_FORMAT_RGBA32 };
        // Used when converting to a TPL texture.
        std::vector<uint32_t> tplColorPalette;

        // TODO: find a better solution for this ^

        Image() = default;
        Image(uint16_t width, uint16_t height, GLuint texture) : width(width), height(height), texture(texture) {}

        bool LoadFromFile(const char* filename) {
            return LoadTextureFromFile(filename, &this->texture, &this->width, &this->height);
        }

        bool LoadFromMem(const unsigned char* buffer, const uint32_t size) {
            return LoadTextureFromMem(buffer, size, &this->texture, &this->width, &this->height);
        }

        void FreeTexture() {
            glDeleteTextures(1, &this->texture);
        }

        std::optional<TPL::TPLTexture> ExportToTPLTexture();

        bool ExportToFile(const char* filename);
    };
}

#endif // COMMON_HPP