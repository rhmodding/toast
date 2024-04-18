#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>

#include <assert.h>

#include <GLFW/glfw3.h>

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

    template <typename T>
    void deleteIfNotNullptr(T* ptr) {
        if (ptr)
            delete ptr;
    }

    float EaseInOut(float t);
    float EaseOut(float t);

    // Simple helper function to load an image into a OpenGL texture with common settings
    bool LoadTextureFromFile(const char* filename, GLuint* texturePtr, int* width, int* height);

    struct Image {
        int width;
        int height;

        GLuint texture;

        bool LoadFromFile(const char* filename) {
            return LoadTextureFromFile(filename, &this->texture, &this->width, &this->height);
        }
    };
}

#endif // COMMON_HPP