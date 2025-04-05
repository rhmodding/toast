#include "Texture.hpp"

#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"

#include "../MainThreadTaskManager.hpp"

#include "../Logging.hpp"

Texture::~Texture() {
    this->DestroyTexture();
}

void Texture::LoadRGBA32(const unsigned char* data, unsigned width, unsigned height) {
    if (data == nullptr) {
        Logging::err << "[Texture::LoadRGBA32] Failed to load image data: data is nullptr" << std::endl;
        return;
    }

    this->width = width;
    this->height = height;

    MainThreadTaskManager::getInstance().QueueTask([this, data]() {
        if (this->textureId == INVALID_TEXTURE_ID)
            glGenTextures(1, &this->textureId);

        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();
}

bool Texture::LoadSTBMem(const unsigned char* data, unsigned dataSize) {
    if (data == nullptr) {
        Logging::err << "[Texture::LoadSTBFile] Failed to load image data: data is nullptr" << std::endl;
        return false;
    }

    int w, h;
    unsigned char* imageData = stbi_load_from_memory(data, dataSize, &w, &h, nullptr, 4);

    if (imageData == nullptr) {
        Logging::err << "[Texture::LoadSTBMem] Failed to load image data from memory location " << data << std::endl;
        return false;
    }

    this->LoadRGBA32(imageData, w, h);

    stbi_image_free(imageData);

    return true;
}

bool Texture::LoadSTBFile(const char* filename) {
    if (filename == nullptr) {
        Logging::err << "[Texture::LoadSTBFile] Failed to export to file \"" << filename << "\": filename is nullptr" << std::endl;
        return false;
    }

    int imageWidth, imageHeight;
    unsigned char* imageData = stbi_load(filename, &imageWidth, &imageHeight, nullptr, 4);

    if (imageData == nullptr) {
        Logging::err << "[Texture::LoadSTBFile] Failed to load image data from file path \"" << filename << "\"!" << std::endl;
        return false;
    }

    this->LoadRGBA32(imageData, imageWidth, imageHeight);

    stbi_image_free(imageData);

    return true;
}

bool Texture::GetRGBA32(unsigned char* buffer) {
    if (buffer == nullptr) {
        Logging::err << "[Texture::GetRGBA32] Failed to download image data: buffer is nullptr" << std::endl;
        return false;
    }

    if (this->textureId == INVALID_TEXTURE_ID) {
        Logging::err << "[Texture::GetRGBA32] Failed to download image data: textureId is invalid" << std::endl;
        return false;
    }

    MainThreadTaskManager::getInstance().QueueTask([this, buffer]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return true;  
}

unsigned char* Texture::GetRGBA32() {
    unsigned char* imageData = new unsigned char[this->width * this->height * 4];

    if (Texture::GetRGBA32(imageData))
        return imageData;
    else {
        delete[] imageData;
        return nullptr;
    }
}

bool Texture::ExportToFile(const char* filename) {
    if (filename == nullptr) {
        Logging::err << "[Texture::ExportToFile] Failed to export to file \"" << filename << "\": filename is nullptr" << std::endl;
        return false;
    }

    unsigned char* imageData = this->GetRGBA32();
    if (imageData == nullptr) {
        Logging::err << "[Texture::ExportToFile] Failed to export to file \"" << filename << "\": GetRGBA32 failed" << std::endl;
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
        Logging::err << "[Texture::ExportToFile] Failed to export to file \"" << filename << "\": stbi_write_png failed" << std::endl;
        return false;
    }

    return true;
}

std::optional<TPL::TPLTexture> Texture::TPLTexture() {
    if (this->textureId == INVALID_TEXTURE_ID) {
        Logging::err << "[Texture::TPLTexture] Failed to construct TPLTexture: textureId is invalid" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    TPL::TPLTexture tplTexture {
        .width = this->width,
        .height = this->height,

        .mipCount = 1, // TODO use true mip count

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

std::optional<CTPK::CTPKTexture> Texture::CTPKTexture() {
    if (this->textureId == INVALID_TEXTURE_ID) {
        Logging::err << "[Texture::CTPKTexture] Failed to construct CTPKTexture: textureId is invalid" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    CTPK::CTPKTexture ctpkTexture {
        .width = this->width,
        .height = this->height,

        .mipCount = this->outputMipCount,

        .format = this->ctpkOutputFormat,

        .sourceTimestamp = 0,

        .data = std::vector<unsigned char>(this->width * this->height * 4)
    };

    MainThreadTaskManager::getInstance().QueueTask([this, &ctpkTexture]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, ctpkTexture.data.data());

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return ctpkTexture;
}

void Texture::DestroyTexture() {
    if (this->textureId == INVALID_TEXTURE_ID)
        return;

    MainThreadTaskManager::getInstance().QueueTask([textureId = this->textureId]() {
        glDeleteTextures(1, &textureId);
    });
    this->textureId = 0;
}