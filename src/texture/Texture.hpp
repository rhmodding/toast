#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "../glInclude.hpp"

#include "TPL.hpp"
#include "CTPK.hpp"

class Texture {
public:
    static constexpr GLuint INVALID_TEXTURE_ID = 0;

private:
    unsigned width { 0 };
    unsigned height { 0 };

    GLuint textureId { INVALID_TEXTURE_ID };

    unsigned outputMipCount { 1 };

    TPL::TPLImageFormat tplOutputFormat { TPL::TPL_IMAGE_FORMAT_RGBA32 };
    CTPK::CTPKImageFormat ctpkOutputFormat { CTPK::CTPK_IMAGE_FORMAT_ETC1A4 };

    std::string name;

public:
    Texture() = default;
    Texture(unsigned width, unsigned height, GLuint textureId) :
        width(width), height(height), textureId(textureId)
    {}

    ~Texture();

    const std::string& getName() const { return this->name; }
    void setName(std::string name) { this->name = name; }

    unsigned getWidth() const { return this->width; }
    unsigned getHeight() const { return this->height; }

    unsigned getPixelCount() const { return this->width * this->height; }

    GLuint getTextureId() const { return this->textureId; }

    TPL::TPLImageFormat getTPLOutputFormat() const { return this->tplOutputFormat; }
    void setTPLOutputFormat(TPL::TPLImageFormat format) { this->tplOutputFormat = format; }

    CTPK::CTPKImageFormat getCTPKOutputFormat() const { return this->ctpkOutputFormat; }
    void setCTPKOutputFormat(CTPK::CTPKImageFormat format) { this->ctpkOutputFormat = format; }

    unsigned getOutputMipCount() const { return this->outputMipCount; }
    void setOutputMipCount(unsigned mipCount) { this->outputMipCount = mipCount; }

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

    // Construct a CTPKTexture from this texture.
    //
    // Returns: CTPK::CTPKTexture wrapped in std::optional
    std::optional<CTPK::CTPKTexture> CTPKTexture();

    // Destroy GPU texture.
    void DestroyTexture();
};

#endif // TEXTURE_HPP
