#include "Texture.hpp"

#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"

#include "../MainThreadTaskManager.hpp"

#include "../Logging.hpp"

Texture::Texture(unsigned width, unsigned height, GLuint textureId) :
    mWidth(width), mHeight(height), mTextureId(textureId)
{
    MainThreadTaskManager::getInstance().QueueTask([this]() {
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &mWrapS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &mWrapT);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &mMinFilter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mMagFilter);
    }).get();
}

Texture::~Texture() {
    DestroyTexture();
}

void Texture::setWrapS(GLint wrapS) {
    MainThreadTaskManager::getInstance().QueueTask([this, wrapS]() {
        mWrapS = wrapS;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    }).get();
}
void Texture::setWrapT(GLint wrapT) {
    MainThreadTaskManager::getInstance().QueueTask([this, wrapT]() {
        mWrapT = wrapT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    }).get();
}

void Texture::setMinFilter(GLint minFilter) {
    MainThreadTaskManager::getInstance().QueueTask([this, minFilter]() {
        mMinFilter = minFilter;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    }).get();
}
void Texture::setMagFilter(GLint magFilter) {
    MainThreadTaskManager::getInstance().QueueTask([this, magFilter]() {
        mMagFilter = magFilter;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, magFilter);
    }).get();
}

void Texture::LoadRGBA32(const unsigned char* data, unsigned width, unsigned height) {
    if (data == nullptr) {
        Logging::err << "[Texture::LoadRGBA32] Failed to load image data: data is NULL" << std::endl;
        return;
    }

    width = width;
    height = height;

    MainThreadTaskManager::getInstance().QueueTask([this, data]() {
        if (mTextureId == INVALID_TEXTURE_ID)
            glGenTextures(1, &mTextureId);

        glBindTexture(GL_TEXTURE_2D, mTextureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

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

    LoadRGBA32(imageData, w, h);

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

    LoadRGBA32(imageData, imageWidth, imageHeight);

    stbi_image_free(imageData);

    return true;
}

bool Texture::GetRGBA32(unsigned char* buffer) {
    if (buffer == nullptr) {
        Logging::err << "[Texture::GetRGBA32] Failed to download image data: buffer is NULL" << std::endl;
        return false;
    }

    if (mTextureId == INVALID_TEXTURE_ID) {
        Logging::err << "[Texture::GetRGBA32] Failed to download image data: textureId is invalid" << std::endl;
        return false;
    }

    MainThreadTaskManager::getInstance().QueueTask([this, buffer]() {
        glBindTexture(GL_TEXTURE_2D, mTextureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return true;  
}

unsigned char* Texture::GetRGBA32() {
    unsigned char* imageData = new unsigned char[mWidth * mHeight * 4];

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

    unsigned char* imageData = GetRGBA32();
    if (imageData == nullptr) {
        Logging::err << "[Texture::ExportToFile] Failed to export to file \"" << filename << "\": GetRGBA32 failed" << std::endl;
        return false;
    }

    int write = stbi_write_png(
        filename.data(), // filename
        mWidth, mHeight, // x, y
        4, // comp
        imageData, // data
        mWidth * 4 // stride_bytes
    );

    delete[] imageData;

    if (!write) {
        Logging::err << "[Texture::ExportToFile] Failed to export to file \"" << filename << "\": stbi_write_png failed" << std::endl;
        return false;
    }

    return true;
}

std::optional<TPL::TPLTexture> Texture::TPLTexture() {
    if (mTextureId == INVALID_TEXTURE_ID) {
        Logging::err << "[Texture::TPLTexture] Failed to construct TPLTexture: textureId is invalid" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    TPL::TPLTexture tplTexture {
        .width = mWidth,
        .height = mHeight,

        .mipCount = 1, // TODO use true mip count

        .format = mTPLOutputFormat,

        .data = std::vector<unsigned char>(mWidth * mHeight * 4)
    };

    tplTexture.minFilter = mMinFilter == GL_NEAREST ?
        TPL::TPL_TEX_FILTER_NEAR :
        TPL::TPL_TEX_FILTER_LINEAR;
    tplTexture.magFilter = mMagFilter == GL_NEAREST ?
        TPL::TPL_TEX_FILTER_NEAR :
        TPL::TPL_TEX_FILTER_LINEAR;

    tplTexture.wrapS = mWrapS == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;
    tplTexture.wrapT = mWrapT == GL_REPEAT ?
        TPL::TPL_WRAP_MODE_REPEAT :
        TPL::TPL_WRAP_MODE_CLAMP;

    MainThreadTaskManager::getInstance().QueueTask([this, &tplTexture]() {
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return tplTexture;
}

std::optional<CTPK::CTPKTexture> Texture::CTPKTexture() {
    if (mTextureId == INVALID_TEXTURE_ID) {
        Logging::err << "[Texture::CTPKTexture] Failed to construct CTPKTexture: textureId is invalid" << std::endl;
        return std::nullopt; // return nothing (std::optional)
    }

    CTPK::CTPKTexture ctpkTexture {
        .width = mWidth,
        .height = mHeight,

        .mipCount = mOutputMipCount,

        .format = mCTPKOutputFormat,

        .sourceTimestamp = 0,

        .data = std::vector<unsigned char>(mWidth * mHeight * 4)
    };

    MainThreadTaskManager::getInstance().QueueTask([this, &ctpkTexture]() {
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, ctpkTexture.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return ctpkTexture;
}

void Texture::DestroyTexture() {
    if (mTextureId == INVALID_TEXTURE_ID)
        return;

    MainThreadTaskManager::getInstance().QueueTask([textureId = mTextureId]() {
        glDeleteTextures(1, &textureId);
    });
    mTextureId = 0;
}