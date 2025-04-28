#include "Texture.hpp"

#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"

#include "../MainThreadTaskManager.hpp"

#include "../Logging.hpp"

Texture::Texture(unsigned width, unsigned height, GLuint textureId) :
    width(width), height(height), textureId(textureId)
{
    MainThreadTaskManager::getInstance().QueueTask([this]() {
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &this->wrapS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &this->wrapT);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &this->minFilter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &this->magFilter);
    }).get();
}

Texture::~Texture() {
    this->DestroyTexture();
}

void Texture::setWrapS(GLint wrapS) {
    MainThreadTaskManager::getInstance().QueueTask([this, wrapS]() {
        this->wrapS = wrapS;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    }).get();
}
void Texture::setWrapT(GLint wrapT) {
    MainThreadTaskManager::getInstance().QueueTask([this, wrapT]() {
        this->wrapT = wrapT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    }).get();
}

void Texture::setMinFilter(GLint minFilter) {
    MainThreadTaskManager::getInstance().QueueTask([this, minFilter]() {
        this->minFilter = minFilter;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    }).get();
}
void Texture::setMagFilter(GLint magFilter) {
    MainThreadTaskManager::getInstance().QueueTask([this, magFilter]() {
        this->magFilter = magFilter;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, magFilter);
    }).get();
}

void Texture::LoadRGBA32(const unsigned char* data, unsigned width, unsigned height) {
    if (data == nullptr) {
        Logging::err << "[Texture::LoadRGBA32] Failed to load image data: data is NULL" << std::endl;
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
        Logging::err << "[Texture::LoadSTBFile] Failed to load image data: data is NULL" << std::endl;
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

bool Texture::LoadSTBFile(std::string_view filename) {
    if (filename.empty()) {
        Logging::err << "[Texture::LoadSTBFile] Failed to export to file: filename is empty" << std::endl;
        return false;
    }

    int imageWidth, imageHeight;
    unsigned char* imageData = stbi_load(filename.data(), &imageWidth, &imageHeight, nullptr, 4);

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
        Logging::err << "[Texture::GetRGBA32] Failed to download image data: buffer is NULL" << std::endl;
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

bool Texture::ExportToFile(std::string_view filename) {
    if (filename.empty()) {
        Logging::err << "[Texture::ExportToFile] Failed to export to file: filename is empty" << std::endl;
        return false;
    }

    unsigned char* imageData = this->GetRGBA32();
    if (imageData == nullptr) {
        Logging::err << "[Texture::ExportToFile] Failed to export to file \"" << filename << "\": GetRGBA32 failed" << std::endl;
        return false;
    }

    int write = stbi_write_png(
        filename.data(), // filename
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

        .format = this->tplOutputFormat,

        .data = std::vector<unsigned char>(this->width * this->height * 4)
    };

    tplTexture.minFilter = this->minFilter == GL_NEAREST ?
        TPL::TPL_TEX_FILTER_NEAR :
        TPL::TPL_TEX_FILTER_LINEAR;
    tplTexture.magFilter = this->magFilter == GL_NEAREST ?
        TPL::TPL_TEX_FILTER_NEAR :
        TPL::TPL_TEX_FILTER_LINEAR;

    tplTexture.wrapS = this->wrapS == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;
    tplTexture.wrapT = this->wrapT == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;

    MainThreadTaskManager::getInstance().QueueTask([this, &tplTexture]() {
        glBindTexture(GL_TEXTURE_2D, this->textureId);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

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