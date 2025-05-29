#ifndef TEXTURE_EX_HPP
#define TEXTURE_EX_HPP

#include "Texture.hpp"

#include "TPL.hpp"
#include "CTPK.hpp"

#include <optional>

/*
    Extended Texture class, used for spritesheets.
*/

class TextureEx : public Texture {
private:
    unsigned mOutputMipCount { 1 };

    TPL::TPLImageFormat mTPLOutputFormat { TPL::TPL_IMAGE_FORMAT_RGBA32 };
    CTPK::CTPKImageFormat mCTPKOutputFormat { CTPK::CTPK_IMAGE_FORMAT_ETC1A4 };

    // CTPK-specific fields.
    uint32_t mOutputSrcTimestamp;
    std::string mOutputSrcPath;

    // Cached ETC1(A4) data.
    //     - Packing ETC1(A4) textures is extremely slow and undeterministic, so to reduce
    //       save time & have deterministic saves we cache the (original) ETC1(A4) data.
    //     - Only used if this texture targets ETC1(A4), otherwise empty.
    std::vector<unsigned char> mCachedETC1;

    std::string mName;

public:
    using Texture::Texture;

    unsigned getOutputMipCount() const { return mOutputMipCount; }
    void setOutputMipCount(unsigned mipCount) { mOutputMipCount = mipCount; }

    TPL::TPLImageFormat getTPLOutputFormat() const { return mTPLOutputFormat; }
    void setTPLOutputFormat(TPL::TPLImageFormat format) { mTPLOutputFormat = format; }

    CTPK::CTPKImageFormat getCTPKOutputFormat() const { return mCTPKOutputFormat; }
    void setCTPKOutputFormat(CTPK::CTPKImageFormat format) { mCTPKOutputFormat = format; }

    uint32_t getOutputSrcTimestamp() const { return mOutputSrcTimestamp; }
    void setOutputSrcTimestamp(uint32_t timestamp) { mOutputSrcTimestamp = timestamp; }

    const std::string& getOutputSrcPath() const { return mOutputSrcPath; }
    void setOutputSrcPath(std::string path) { mOutputSrcPath = path; }

    const std::string& getName() const { return mName; }
    void setName(std::string name) { mName = name; }

    // Construct a TPLTexture from this texture.
    //
    // Returns: TPL::TPLTexture wrapped in std::optional
    [[nodiscard]] std::optional<TPL::TPLTexture> TPLTexture();

    // Construct a CTPKTexture from this texture.
    //
    // Returns: CTPK::CTPKTexture wrapped in std::optional
    [[nodiscard]] std::optional<CTPK::CTPKTexture> CTPKTexture();
};

#endif // TEXTURE_EX_HPP
