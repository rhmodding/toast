#include "common.hpp"

#include "stb/stb_image.h"

namespace Common {
    constexpr uint32_t magicToUint32(const char* magic, bool bigEndian) {
        uint32_t result = 0;

        for (size_t i = 0; i < 4; ++i) {
            size_t shift = bigEndian ? (8 * (3 - i)) : (8 * i);
            result |= static_cast<uint32_t>(magic[i]) << shift;
        }

        return result;
    }

    bool LoadTextureFromFile(const char* filename, GLuint* texturePtr, int* width, int* height)  {
        // Load from file
        int imageWidth{ 0 };
        int imageHeight{ 0 };

        unsigned char* imagePtr = stbi_load(filename, &imageWidth, &imageHeight, nullptr, 4);
        if (imagePtr == nullptr)
            return false;

        // Create a OpenGL texture identifier
        GLuint imageTexture;
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePtr);
        stbi_image_free(imagePtr);

        *texturePtr = imageTexture;

        if (width != nullptr)
            *width = imageWidth;
        if (height != nullptr)
            *height = imageHeight;

        return true;
    }

    float EaseInOut(float t) {
        return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
    }
    float EaseOut(float t) {
        return 1 - (1 - t) * (1 - t);
    }
}
