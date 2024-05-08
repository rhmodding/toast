#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>

#undef NDEBUG
#include <assert.h>

#include <GLFW/glfw3.h>

#include "stb/stb_image_write.h"

#include "imgui.h"

#define STB_IMAGE_IMPLEMENTATION

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
    template <typename T>
    void ReadAtOffset(const char* source, size_t& offset, const size_t sourceSize, T& buffer) {
        // Read will cause out-of-bounds access
        assert((offset - 1 + sizeof(T)) < sourceSize);

        std::memcpy(&buffer, source + offset, sizeof(T));
        offset += sizeof(T);
    }

    template <typename T>
    T beRightShift(T num, int shift) {
        T numCpy = num;
        
        numCpy = BYTESWAP_32(numCpy);

        numCpy = numCpy >> shift;

        numCpy = BYTESWAP_32(numCpy);

        return num;
    }

    constexpr uint32_t magicToUint32(const char* magic, bool bigEndian = false);

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

    void fitRectangle(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale);

    float EaseInOut(float t);
    float EaseOut(float t);

    constexpr ImVec4 RGBAtoImVec4(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }

    // Simple helper function to load an image into a OpenGL texture with common settings
    bool LoadTextureFromFile(const char* filename, GLuint* texturePtr, int* width, int* height);

    struct Image {
        int width;
        int height;

        GLuint texture;

        Image() = default;
        Image(uint16_t width, uint16_t height, GLuint texture) : width(width), height(height), texture(texture) {}

        bool LoadFromFile(const char* filename) {
            return LoadTextureFromFile(filename, &this->texture, &this->width, &this->height);
        }

        void FreeTexture() {
            glDeleteTextures(1, &this->texture);
        }

        bool ExportToFile(const char* filename) {
            if (!this->texture)
                return false;

            glBindTexture(GL_TEXTURE_2D, this->texture);

            unsigned char* data = new unsigned char[
                this->width *
                this->height *
                4
            ]; // 4 bytes per pixel (RGBA)

            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            glBindTexture(GL_TEXTURE_2D, 0);

            // Write buffer data to PNG file
            int write = stbi_write_png(
                filename,
                this->width,
                this->height,
                4,
                data,
                0
            );

            if (!write)
                std::cerr << "[Common::Image::ExportToFile] Failed to write PNG to path: " << filename << '\n';

            delete[] data;

            return write > 0;
        }
    };
}

#endif // COMMON_HPP