#include "common.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <fstream>

#include <cmath>

#include <bit>

#include "MtCommandManager.hpp"

namespace Common {

float byteswapFloat(float value) {
    return std::bit_cast<float, uint32_t>(
        BYTESWAP_32(std::bit_cast<uint32_t, float>(value))
    );
}

bool SaveBackupFile(const char* filePath, bool once) {
    char newFileName[PATH_MAX];
    snprintf(newFileName, PATH_MAX, "%s.bak", filePath);

    bool exists{ false };
    {
        if (FILE *file = fopen(newFileName, "r")) {
            fclose(file);
            exists = true;
        }
    }

    if (once && exists)
        return true;

    std::ifstream src(filePath,    std::ios::binary);
    std::ofstream dst(newFileName, std::ios::binary);

    if (UNLIKELY(!src.is_open())) {
        std::cerr << "[Common::SaveBackupFile] Error opening source file at path: " << filePath << '\n';
        return false;
    }
    if (UNLIKELY(!dst.is_open())) {
        std::cerr << "[Common::SaveBackupFile] Error opening destination file at path: " << newFileName << '\n';
        return false;
    }

    dst << src.rdbuf();

    src.close();
    dst.close();

    std::cout << "[Common::SaveBackupFile] Successfully cloned file at path: " << filePath << '\n';

    return true;
}

std::string randomString(const unsigned length) {
    static const char characters[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string string;
    string.resize(length);

    for (unsigned i = 0; i < length; i++)
        string.at(i) =
            characters[rand() % (sizeof(characters) - 1)];

    return string;
}

bool LoadTextureFromFile(const char* filename, GLuint* texturePtr, int* outWidth, int* outHeight)  {
    int imageWidth{ 0 };
    int imageHeight{ 0 };

    unsigned char* imagePtr = stbi_load(filename, &imageWidth, &imageHeight, nullptr, 4);
    if (imagePtr == nullptr) {
        std::cerr << "[Common::LoadTextureFromFile] Failed to load image from path: " << filename << '\n';
        return false;
    }

    GLuint imageTexture;
    std::future<void> future = MtCommandManager::getInstance().enqueueCommand([&imageTexture, imageWidth, imageHeight, imagePtr]() {
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePtr);

        glBindTexture(GL_TEXTURE_2D, 0);
    });

    future.get();

    stbi_image_free(imagePtr);

    *texturePtr = imageTexture;

    if (outWidth != nullptr)
        *outWidth = imageWidth;
    if (outHeight != nullptr)
        *outHeight = imageHeight;

    return true;
}

bool LoadTextureFromMem(const unsigned char* buffer, const uint32_t size, GLuint* texturePtr, int* outWidth, int* outHeight) {
    int imageWidth{ 0 };
    int imageHeight{ 0 };

    unsigned char* imagePtr = stbi_load_from_memory(buffer, size, &imageWidth, &imageHeight, nullptr, 4);
    if (imagePtr == nullptr) {
        std::cerr << "[Common::LoadTextureFromFile] Failed to load image from memory location " << buffer << '\n';
        return false;
    }

    GLuint imageTexture;
    std::future<void> future = MtCommandManager::getInstance().enqueueCommand([&imageTexture, imageWidth, imageHeight, imagePtr]() {
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePtr);

        glBindTexture(GL_TEXTURE_2D, 0);
    });

    future.get();

    stbi_image_free(imagePtr);

    *texturePtr = imageTexture;

    if (outWidth != nullptr)
        *outWidth = imageWidth;
    if (outHeight != nullptr)
        *outHeight = imageHeight;

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

bool IsMouseInRegion(const ImVec2 point, float radius) {
    const ImVec2 mousePos = ImGui::GetMousePos();

    const float distance = sqrtf(
        powf(mousePos.x - point.x, 2) +
        powf(mousePos.y - point.y, 2)
    );

    return distance <= radius;
}

std::optional<TPL::TPLTexture> Image::ExportToTPLTexture() {
    if (!(
        this->width != 0 &&
        this->height != 0 &&
        this->texture
    ))
        return std::nullopt; // return nothing (std::optional)

    TPL::TPLTexture tplTexture;
    tplTexture.data.resize(this->width * this->height * 4);

    GLint wrapModeS, wrapModeT;

    {
        GLuint texture = this->texture;
        unsigned char* textureBuffer = tplTexture.data.data();
        std::future<void> future = MtCommandManager::getInstance().enqueueCommand([texture, textureBuffer, &wrapModeS, &wrapModeT]() {
            glBindTexture(GL_TEXTURE_2D, texture);

            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureBuffer);

            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapModeS);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrapModeT);

            glBindTexture(GL_TEXTURE_2D, 0);
        });

        future.get();
    }

    tplTexture.width = this->width;
    tplTexture.height = this->height;

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

    return tplTexture;
}

bool Image::ExportToFile(const char* filename) {
    if (!(
        this->width != 0 &&
        this->height != 0 &&
        this->texture
    ))
        return false;

    unsigned char* data = new unsigned char[
        this->width *
        this->height *
        4
    ]; // 4 bytes per pixel (RGBA)

    {
        GLuint texture = this->texture;
        std::future<void> future = MtCommandManager::getInstance().enqueueCommand([texture, data]() {
            glBindTexture(GL_TEXTURE_2D, texture);

            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            glBindTexture(GL_TEXTURE_2D, 0);
        });

        future.get();
    }

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

} // namespace Common
