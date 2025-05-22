#include "TextureGroup.hpp"

std::shared_ptr<Texture>& TextureGroup::getBaseTexture() {
    return mVec[mBaseTextureIndex % mVec.size()];
}

std::shared_ptr<Texture>& TextureGroup::getTextureByVarying(unsigned varying) {
    unsigned index = mBaseTextureIndex;
    if (mVaryingEnabled)
        index += varying;

    return mVec[index % mVec.size()];
}

std::shared_ptr<Texture>& TextureGroup::getTextureByIndex(unsigned index) {
    return mVec[index % mVec.size()];
}

unsigned TextureGroup::addTexture(std::shared_ptr<Texture> texture) {
    mVec.push_back(texture);
    return mVec.size() - 1;
}

void TextureGroup::removeTexture(unsigned textureIndex) {
    if (textureIndex >= mVec.size())
        return;

    auto textureIt = mVec.begin() + textureIndex;
    mVec.erase(textureIt);
}
