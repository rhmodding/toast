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

    mWidth = width;
    mHeight = height;

    MainThreadTaskManager::getInstance().QueueTask([this, data]() {
        if (mTextureId == INVALID_TEXTURE_ID)
            glGenTextures(1, &mTextureId);

        glBindTexture(GL_TEXTURE_2D, mTextureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mMagFilter);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();
}

bool Texture::LoadSTBMem(const unsigned char* data, int dataSize) {
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

void Texture::DestroyTexture() {
    if (mTextureId == INVALID_TEXTURE_ID)
        return;

    MainThreadTaskManager::getInstance().QueueTask([textureId = mTextureId]() {
        glDeleteTextures(1, &textureId);
    });
    mTextureId = 0;
}