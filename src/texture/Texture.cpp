#include "Texture.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#include <future>
#include "../MtCommandManager.hpp"

Texture::~Texture() {
    if (this->textureId == 0)
        return;

    MtCommandManager::getInstance().enqueueCommand([textureId = this->textureId]() {
        glDeleteTextures(1, &textureId);
    });
}

void Texture::LoadRGBA32(const unsigned char* data, unsigned width, unsigned height) {
    this->width = width;
    this->height = height;

    std::future<void> future = MtCommandManager::getInstance().enqueueCommand([this, data]() {
        if (this->textureId == 0)
            glGenTextures(1, &this->textureId);

        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, 0);
    });
    future.get();
}

bool Texture::LoadSTBMem(const unsigned char* data, unsigned dataSize) {
    int imageWidth, imageHeight;
    unsigned char* imageData = stbi_load_from_memory(data, dataSize, &imageWidth, &imageHeight, nullptr, 4);

    if (imageData == nullptr) {
        std::cerr << "[Texture::LoadSTBMem] Failed to load image data from memory location " << data << '\n';
        return false;
    }

    this->LoadRGBA32(imageData, imageWidth, imageHeight);

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

    std::future<void> future = MtCommandManager::getInstance().enqueueCommand([this, imageData]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

        glBindTexture(GL_TEXTURE_2D, 0);
    });
    future.get();

    return imageData;
}

bool Texture::ExportToFile(const char* filename) {
    unsigned char* imageData = this->GetRGBA32();
    if (imageData == nullptr) {
        std::cerr << "[Texture::ExportToFile] Failed to export to file (\"" << filename << "\"): GetRGBA32 failed\n";
        return false;
    }

    int write = stbi_write_png(
        filename,
        this->width, this->height,
        4,
        imageData,
        0
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

        .width = (uint16_t)this->width,
        .height = (uint16_t)this->height,

        .minFilter = TPL::TPL_TEX_FILTER_LINEAR,
        .magFilter = TPL::TPL_TEX_FILTER_LINEAR,

        .format = this->tplOutputFormat
    };
    tplTexture.data.resize(this->width * this->height * 4);

    GLint wrapModes[2];

    std::future<void> future = MtCommandManager::getInstance().enqueueCommand([this, &tplTexture, &wrapModes]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());

        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapModes + 0);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapModes + 1);

        glBindTexture(GL_TEXTURE_2D, 0);
    });
    future.get();

    tplTexture.wrapS = wrapModes[0] == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;
    tplTexture.wrapT = wrapModes[1] == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;

    return tplTexture;
}

void Texture::DestroyTexture() {
    if (this->textureId == 0)
        return;

    MtCommandManager::getInstance().enqueueCommand([textureId = this->textureId]() {
        glDeleteTextures(1, &textureId);
    });
}