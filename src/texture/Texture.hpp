#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "../glInclude.hpp"

#include "TPL.hpp"

class Texture {
private:
    unsigned width { 0 };
    unsigned height { 0 };

    GLuint textureId { 0 };

    TPL::TPLImageFormat tplOutputFormat { TPL::TPL_IMAGE_FORMAT_RGBA32 };
public:
    Texture() = default;
    Texture(unsigned width, unsigned height, GLuint textureId) :
        width(width), height(height), textureId(textureId)
    {}

    ~Texture();

    unsigned getWidth() const { return this->width; }
    unsigned getHeight() const { return this->height; }

    // width * height
    unsigned getPixelCount() const { return this->width * this->height; }

    GLuint getTextureId() const { return this->textureId; }

    TPL::TPLImageFormat getTPLOutputFormat() const { return this->tplOutputFormat; }
    void setTPLOutputFormat(TPL::TPLImageFormat format) { this->tplOutputFormat = format; };

    // Generate a texture & upload the RGBA32 data to it.
    // Note: if a GPU texture already exists, this will overwrite it's data.
    void LoadRGBA32(const unsigned char* data, unsigned width, unsigned height);

    // Generate a texture & use stb_image to load the image data from memory.
    // Note: if a GPU texture already exists, this will overwrite it's data.
    //
    // Returns: true if succeeded, false if failed
    bool LoadSTBMem(const unsigned char* data, unsigned dataSize);

    // Generate a texture & use stb_image to load the image data from the filesystem.
    // Note: if a GPU texture already exists, this will overwrite it's data.
    //
    // Returns: true if succeeded, false if failed
    bool LoadSTBFile(const char* filename);

    // Download the texture from the GPU as RGBA32 image data.
    // Note A: the resulting pointer is dynamically allocated and must be freed by the caller.
    // Note B: the size of the data is this->getPixelCount() * 4
    //
    // Returns: pointer to linear RGBA32 image data if succeeded, nullptr if failed
    unsigned char* GetRGBA32();

    // Download the texture from the GPU into a RGBA32 image buffer.
    // Note: the size of the buffer must be this->getPixelCount() * 4
    // 
    // Returns: true if succeeded, false if failed
    bool GetRGBA32(unsigned char* buffer);

    // Export the texture to the filesystem using stb_image_write.
    //
    // Returns: true if succeeded, false if failed
    bool ExportToFile(const char* filename);

    // Construct a TPLTexture from this texture.
    //
    // Returns: TPL::TPLTexture wrapped in std::optional
    std::optional<TPL::TPLTexture> TPLTexture();

    // Destroy GPU texture.
    void DestroyTexture();
};

#endif // TEXTURE_HPP
