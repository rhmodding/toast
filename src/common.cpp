#include "common.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace Common {
    float readBigEndianFloat(const uint8_t* bytes) {
        uint32_t value = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(bytes));
        return *reinterpret_cast<float*>(&value);
    }

    float byteswapFloat(float value) {
        uint32_t rValue = BYTESWAP_32(*reinterpret_cast<const uint32_t*>(&value));
        return *reinterpret_cast<float*>(&value);
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

        glBindTexture(GL_TEXTURE_2D, 0);

        *texturePtr = imageTexture;

        if (width != nullptr)
            *width = imageWidth;
        if (height != nullptr)
            *height = imageHeight;

        return true;
    }

    bool LoadTextureFromMem(const unsigned char* buffer, const uint32_t size, GLuint* texturePtr, int* width, int* height) {
        // Load from memory
        int imageWidth{ 0 };
        int imageHeight{ 0 };

        unsigned char* imagePtr = stbi_load_from_memory(buffer, size, &imageWidth, &imageHeight, nullptr, 4);
        if (imagePtr == nullptr) {
            std::cerr << "[Common::LoadTextureFromFile] Failed to load image from memory location " << buffer << '\n';
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

        glBindTexture(GL_TEXTURE_2D, 0);

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
        return t * t;
    }
    float EaseOut(float t) {
        return 1 - (1 - t) * (1 - t);
    }

    std::optional<TPL::TPLTexture> Image::ExportToTPLTexture() {
        if (!(
            this->width != 0 &&
            this->height != 0 &&
            this->texture
        ))
            return std::nullopt; // return nothing (std::optional)

        glBindTexture(GL_TEXTURE_2D, this->texture);

        TPL::TPLTexture tplTexture;
        tplTexture.data.resize(
            this->width *
            this->height *
            4
        );

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());

        tplTexture.width = this->width;
        tplTexture.height = this->height;

        GLint wrapModeS, wrapModeT;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapModeS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrapModeT);

        glBindTexture(GL_TEXTURE_2D, 0);

        tplTexture.wrapS = wrapModeS == GL_REPEAT ?
            TPL::TPL_WRAP_MODE_REPEAT :
            TPL::TPL_WRAP_MODE_CLAMP;
        tplTexture.wrapT = wrapModeT == GL_REPEAT ?
            TPL::TPL_WRAP_MODE_REPEAT :
            TPL::TPL_WRAP_MODE_CLAMP;

        tplTexture.minFilter = TPL::TPL_TEX_FILTER_LINEAR;
        tplTexture.magFilter = TPL::TPL_TEX_FILTER_LINEAR;

        tplTexture.mipMap = 1;

        tplTexture.format = this->tplOutFormat;

        tplTexture.palette = this->tplColorPalette;

        tplTexture.valid = 0xFFu;

        return tplTexture;
    }

    bool Image::ExportToFile(const char* filename) {
        if (!(
            this->width != 0 &&
            this->height != 0 &&
            this->texture
        ))
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
}
