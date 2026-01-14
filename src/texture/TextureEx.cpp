#include "TextureEx.hpp"

#include "manager/MainThreadTaskManager.hpp"

#include "Logging.hpp"

std::optional<TPL::TPLTexture> TextureEx::TPLTexture() {
    if (mTextureId == INVALID_TEXTURE_ID) {
        Logging::error("[TextureEx::TPLTexture] Failed to construct TPLTexture: textureId is invalid");
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

    MainThreadTaskManager::getInstance().queueTask([this, &tplTexture]() {
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return tplTexture;
}

std::optional<CTPK::CTPKTexture> TextureEx::CTPKTexture() {
    if (mTextureId == INVALID_TEXTURE_ID) {
        Logging::error("[TextureEx::CTPKTexture] Failed to construct CTPKTexture: textureId is invalid");
        return std::nullopt; // return nothing (std::optional)
    }

    CTPK::CTPKTexture ctpkTexture {
        .width = mWidth,
        .height = mHeight,

        .mipCount = mOutputMipCount,

        .targetFormat = mCTPKOutputFormat,

        .sourceTimestamp = mOutputSrcTimestamp,
        .sourcePath = mOutputSrcPath,

        // .cachedTargetData = mCachedETC1,
        .data = std::vector<unsigned char>(mWidth * mHeight * 4)
    };

    if (CTPK::getImageFormatCompressed(mCTPKOutputFormat))
        ctpkTexture.cachedTargetData = mCachedETC1;
    else
        ctpkTexture.cachedTargetData = std::vector<unsigned char>();

    MainThreadTaskManager::getInstance().queueTask([this, &ctpkTexture]() {
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, ctpkTexture.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    return ctpkTexture;
}
