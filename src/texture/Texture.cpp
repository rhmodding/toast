#include "Texture.hpp"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "manager/MainThreadTaskManager.hpp"

#include "Logging.hpp"

Texture::Texture(unsigned width, unsigned height, GLuint textureId) :
    mWidth(width), mHeight(height), mTextureId(textureId)
{
    MainThreadTaskManager::getInstance().queueTask([this]() {
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &mWrapS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &mWrapT);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &mMinFilter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mMagFilter);
    }).get();
}

Texture::~Texture() {
    destroyTexture();
}

void Texture::setWrapS(GLint wrapS) {
    MainThreadTaskManager::getInstance().queueTask([this, wrapS]() {
        mWrapS = wrapS;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    }).get();
}
void Texture::setWrapT(GLint wrapT) {
    MainThreadTaskManager::getInstance().queueTask([this, wrapT]() {
        mWrapT = wrapT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    }).get();
}

void Texture::setMinFilter(GLint minFilter) {
    MainThreadTaskManager::getInstance().queueTask([this, minFilter]() {
        mMinFilter = minFilter;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    }).get();
}
void Texture::setMagFilter(GLint magFilter) {
    MainThreadTaskManager::getInstance().queueTask([this, magFilter]() {
        mMagFilter = magFilter;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, magFilter);
    }).get();
}

void Texture::loadRGBA32(const unsigned char *data, unsigned width, unsigned height) {
    if (data == nullptr) {
        Logging::error("[Texture::loadRGBA32] Failed to load image data: data is NULL");
        return;
    }

    mWidth = width;
    mHeight = height;

    MainThreadTaskManager::getInstance().queueTask([this, data]() {
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

bool Texture::loadSTBMem(const unsigned char *data, int dataSize) {
    if (data == nullptr) {
        Logging::error("[Texture::loadSTBMem] Failed to load image data: data is NULL");
        return false;
    }

    int w, h;
    unsigned char *imageData = stbi_load_from_memory(data, dataSize, &w, &h, nullptr, 4);

    if (imageData == nullptr) {
        Logging::error("[Texture::loadSTBMem] Failed to load image data from memory location {}", reinterpret_cast<const void *>(data));
        return false;
    }

    loadRGBA32(imageData, w, h);

    stbi_image_free(imageData);

    return true;
}

bool Texture::loadSTBFile(std::string_view filename) {
    if (filename.empty()) {
        Logging::error("[Texture::loadSTBFile] Failed to export to file: filename is empty");
        return false;
    }

    int imageWidth, imageHeight;
    unsigned char *imageData = stbi_load(filename.data(), &imageWidth, &imageHeight, nullptr, 4);

    if (imageData == nullptr) {
        Logging::error("[Texture::loadSTBFile] Failed to load image data from file path \"{}\"!", filename);
        return false;
    }

    loadRGBA32(imageData, imageWidth, imageHeight);

    stbi_image_free(imageData);

    return true;
}

bool Texture::getRGBA32(unsigned char* buffer) {
    if (buffer == nullptr) {
        Logging::error("[Texture::getRGBA32] Failed to download image data: buffer is NULL");
        return false;
    }

    if (mTextureId == INVALID_TEXTURE_ID) {
        Logging::error("[Texture::getRGBA32] Failed to download image data: textureId is invalid");
        return false;
    }

    MainThreadTaskManager::getInstance().queueTask([this, buffer]() {
        glBindTexture(GL_TEXTURE_2D, mTextureId);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return true;
}

unsigned char* Texture::getRGBA32() {
    unsigned char *imageData = new unsigned char[mWidth * mHeight * 4];

    if (Texture::getRGBA32(imageData)) {
        return imageData;
    }
    else {
        delete[] imageData;
        return nullptr;
    }
}

bool Texture::exportToFile(std::string_view filename) {
    if (filename.empty()) {
        Logging::error("[Texture::exportToFile] Failed to export to file: filename is empty");
        return false;
    }

    unsigned char *imageData = getRGBA32();
    if (imageData == nullptr) {
        Logging::error("[Texture::exportToFile] Failed to export to file \"{}\": getRGBA32 failed", filename);
        return false;
    }

    int write;
    if (filename.ends_with(".tga")) {
        write = stbi_write_tga(
            filename.data(), // filename
            mWidth, mHeight, // w, h
            4, // comp
            imageData // data
        );
    }
    else {
        write = stbi_write_png(
            filename.data(), // filename
            mWidth, mHeight, // w, h
            4, // comp
            imageData, // data
            mWidth * 4 // stride_in_bytes
        );
    }

    delete[] imageData;

    if (write == 0) {
        Logging::error("[Texture::exportToFile] Failed to export to file \"{}\": stbi_write_png failed", filename);
        return false;
    }

    return true;
}

void Texture::destroyTexture() {
    if (mTextureId == INVALID_TEXTURE_ID)
        return;

    MainThreadTaskManager::getInstance().queueTask([textureId = mTextureId]() {
        glDeleteTextures(1, &textureId);
    });
    mTextureId = 0;
}