#include "Texture.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#include <future>
#include "../MainThreadTaskManager.hpp"

Texture::~Texture() {
    this->DestroyTexture();
}

void Texture::LoadRGBA32(const unsigned char* data, unsigned width, unsigned height) {
    this->width = width;
    this->height = height;

    MainThreadTaskManager::getInstance().QueueTask([this, data]() {
        if (this->textureId == 0)
            glGenTextures(1, &this->textureId);

        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();
}

bool Texture::LoadSTBMem(const unsigned char* data, unsigned dataSize) {
    int w, h;
    unsigned char* imageData = stbi_load_from_memory(data, dataSize, &w, &h, nullptr, 4);

    if (imageData == nullptr) {
        std::cerr << "[Texture::LoadSTBMem] Failed to load image data from memory location " << data << '\n';
        return false;
    }

    this->LoadRGBA32(imageData, w, h);

    stbi_image_free(imageData);

    return true;
}

bool Texture::LoadSTBFile(const char* filename) {
    int imageWidth, imageHeight;
    unsigned char* imageData = stbi_load(filename, &imageWidth, &imageHeight, nullptr, 4);

    if (imageData == nullptr) {
        std::cerr << "[Texture::LoadSTBFile] Failed to load image data from file path \"" << filename << "\"\n";
        return false;
    }

    this->LoadRGBA32(imageData, imageWidth, imageHeight);

    stbi_image_free(imageData);

    return true;
}

unsigned char* Texture::GetRGBA32() {
    if (this->textureId == 0) {
        std::cerr << "[Texture::GetRGBA32] Failed to download image data: textureId is 0\n";
        return nullptr;
    }

    unsigned char* imageData = new unsigned char[this->width * this->height * 4];
    if (imageData == nullptr) {
        std::cerr << "[Texture::GetRGBA32] Failed to download image data: allocation failed\n";
        return nullptr;
    }

    MainThreadTaskManager::getInstance().QueueTask([this, imageData]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return imageData;
}

bool Texture::GetRGBA32(unsigned char* buffer) {
    if (this->textureId == 0) {
        std::cerr << "[Texture::GetRGBA32] Failed to download image data: textureId is 0\n";
        return false;
    }

    MainThreadTaskManager::getInstance().QueueTask([this, buffer]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return true;  
}

bool Texture::ExportToFile(const char* filename) {
    unsigned char* imageData = this->GetRGBA32();
    if (imageData == nullptr) {
        std::cerr << "[Texture::ExportToFile] Failed to export to file (\"" << filename << "\"): GetRGBA32 failed\n";
        return false;
    }

    int write = stbi_write_png(
        filename, // filename
        this->width, this->height, // x, y
        4, // comp
        imageData, // data
        this->width * 4 // stride_bytes
    );

    delete[] imageData;

    if (!write) {
        std::cerr << "[Texture::ExportToFile] Failed to export to file (" << filename << "): stbi_write_png failed\n";
        return false;
    }

    return true;
}

std::optional<TPL::TPLTexture> Texture::TPLTexture() {
    if (this->textureId == 0) {
        std::cerr << "[Texture::TPLTexture] Failed to construct TPLTexture: textureId is 0\n";
        return std::nullopt; // return nothing (std::optional)
    }

    TPL::TPLTexture tplTexture {
        .mipMap = 1,

        .width = static_cast<uint16_t>(this->width),
        .height = static_cast<uint16_t>(this->height),

        .minFilter = TPL::TPL_TEX_FILTER_LINEAR,
        .magFilter = TPL::TPL_TEX_FILTER_LINEAR,

        .format = this->tplOutputFormat,

        .data = std::vector<unsigned char>(this->width * this->height * 4)
    };

    GLint wrapS, wrapT;

    MainThreadTaskManager::getInstance().QueueTask([this, &tplTexture, &wrapS, &wrapT]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());

        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrapT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    tplTexture.wrapS = wrapS == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;
    tplTexture.wrapT = wrapT == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;

    return tplTexture;
}

void Texture::DestroyTexture() {
    if (this->textureId == 0)
        return;

    MainThreadTaskManager::getInstance().QueueTask([textureId = this->textureId]() {
        glDeleteTextures(1, &textureId);
    });
    this->textureId = 0;
}