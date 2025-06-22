#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "glInclude.hpp"

#include <string_view>

#include <imgui.h> // for ImTextureID

/*
    Generic Texture class
*/

class Texture {
public:
    static constexpr GLuint INVALID_TEXTURE_ID = 0;

protected:
    unsigned mWidth { 0 };
    unsigned mHeight { 0 };

    GLuint mTextureId { INVALID_TEXTURE_ID };

    GLint mWrapS { GL_REPEAT }, mWrapT { GL_REPEAT };
    GLint mMinFilter { GL_LINEAR }, mMagFilter { GL_LINEAR };

public:
    // NOTE: once you construct a Texture with a textureId, you grant it
    // ownership of that GPU texture
    Texture() = default;
    Texture(unsigned width, unsigned height, GLuint textureId);

    virtual ~Texture();

    unsigned getWidth() const { return mWidth; }
    unsigned getHeight() const { return mHeight; }

    unsigned getPixelCount() const { return mWidth * mHeight; }

    GLuint getTextureId() const { return mTextureId; }
    ImTextureID getImTextureId() const { return static_cast<ImTextureID>(mTextureId); }

    GLint getWrapS() const { return mWrapS; }
    void setWrapS(GLint wrapS);

    GLint getWrapT() const { return mWrapT; }
    void setWrapT(GLint wrapT);

    GLint getMinFilter() const { return mMinFilter; }
    void setMinFilter(GLint minFilter);

    GLint getMagFilter() const { return mMagFilter; }
    void setMagFilter(GLint magFilter);

    // Generate a texture & upload the RGBA32 data to it.
    // Note: if a GPU texture already exists, this will overwrite its data.
    void LoadRGBA32(const unsigned char* data, unsigned width, unsigned height);

    // Generate a texture & use stb_image to load the image data from memory.
    //     - Note: if a GPU texture already exists, this will overwrite its data.
    //
    // Returns: true if succeeded, false if failed
    bool LoadSTBMem(const unsigned char* data, int dataSize);

    // Generate a texture & use stb_image to load the image data from the filesystem.
    //     - Note: if a GPU texture already exists, this will overwrite its data.
    //
    // Returns: true if succeeded, false if failed
    bool LoadSTBFile(std::string_view filename);

    // Download the texture from the GPU into a RGBA32 image buffer.
    //     - Note: the size of the buffer must be getPixelCount() * 4
    // 
    // Returns: true if succeeded, false if failed
    bool GetRGBA32(unsigned char* buffer);

    // Download the texture from the GPU as RGBA32 image data.
    //     - Note A: the resulting pointer is dynamically allocated and must be freed by the caller.
    //     - Note B: the size of the data is getPixelCount() * 4
    //
    // Returns: pointer to linear RGBA32 image data if succeeded, nullptr if failed
    [[nodiscard]] unsigned char* GetRGBA32();

    // Export the texture to the filesystem using stb_image_write.
    //
    // Returns: true if succeeded, false if failed
    bool ExportToFile(std::string_view filename);

    // Destroy GPU texture.
    void DestroyTexture();
};

#endif // TEXTURE_HPP
