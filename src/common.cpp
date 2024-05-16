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

    float readBigEndianFloat(const uint8_t* bytes) {
        uint32_t intValue = (static_cast<uint32_t>(bytes[0]) << 24) |
                            (static_cast<uint32_t>(bytes[1]) << 16) |
                            (static_cast<uint32_t>(bytes[2]) << 8) |
                            static_cast<uint32_t>(bytes[3]);

        float result;
        std::memcpy(&result, &intValue, sizeof(float));
        
        return result;
    }

    void writeBigEndianFloat(float value, uint8_t* bytes) {
        uint32_t intValue;
        std::memcpy(&intValue, &value, sizeof(float));

        bytes[0] = static_cast<uint8_t>(intValue >> 24);
        bytes[1] = static_cast<uint8_t>(intValue >> 16);
        bytes[2] = static_cast<uint8_t>(intValue >> 8);
        bytes[3] = static_cast<uint8_t>(intValue);
    }
    
    std::string randomString(const uint32_t length) {
        static const char characters[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        std::string string;
        string.resize(length);

        for (uint32_t i = 0; i < length; i++)
            string.at(i) =
                characters[rand() % (sizeof(characters) - 1)];
        
        return string;
    }

    bool LoadTextureFromFile(const char* filename, GLuint* texturePtr, int* width, int* height)  {
        // Load from file
        int imageWidth{ 0 };
        int imageHeight{ 0 };

        unsigned char* imagePtr = stbi_load(filename, &imageWidth, &imageHeight, nullptr, 4);
        if (imagePtr == nullptr) {
            std::cerr << "[Common::LoadTextureFromFile] Failed to load image from path: " << filename << '\n';
            return false;
        }

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

    void fitRectangle(ImVec2 &rectToFit, const ImVec2 &targetRect, float& scale) {
        float widthRatio = targetRect.x / rectToFit.x;
        float heightRatio = targetRect.y / rectToFit.y;
        
        scale = std::min(widthRatio, heightRatio);
        
        rectToFit.x *= scale;
        rectToFit.y *= scale;
    }

    float EaseInOut(float t) {
        return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
    }
    float EaseIn(float t) {
        if (t == 0.f)
            return 0.f; // Avoid division by 0 exception
        
        return t * t;
    }
    float EaseOut(float t) {
        return 1 - (1 - t) * (1 - t);
    }
}
