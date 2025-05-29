#ifndef TEXTURE_GROUP_HPP
#define TEXTURE_GROUP_HPP

#include <vector>

#include <memory>

#include "Texture.hpp"

template<typename T>
class TextureGroup {
    static_assert(std::is_base_of<Texture, T>::value, "T must be derived from Texture");
public:
    using TexSharedPtr = std::shared_ptr<T>;

private:
    std::vector<TexSharedPtr> mVec;
    unsigned mBaseTextureIndex { 0 };

    bool mVaryingEnabled { false };

public:
    TextureGroup() = default;
    ~TextureGroup() = default;

    bool getVaryingEnabled() const { return mVaryingEnabled; }
    void setVaryingEnabled(bool enabled) { mVaryingEnabled = enabled; }

    TexSharedPtr& getBaseTexture() {
        return mVec[mBaseTextureIndex % mVec.size()];
    }

    unsigned getBaseTextureIndex() const { return mBaseTextureIndex; }
    void setBaseTextureIndex(unsigned index) { mBaseTextureIndex = index; }

    TexSharedPtr& getTextureByVarying(unsigned varying) {
        unsigned index = mBaseTextureIndex;
        if (mVaryingEnabled)
            index += varying;

        return mVec[index % mVec.size()];
    }

    // Gets texture at specified index.
    // Note: out of bounds values will wrap around.
    TexSharedPtr& getTextureByIndex(unsigned index) {
        return mVec[index % mVec.size()];
    }

    // Add a texture to the group.
    //
    // Returns: the index of the new texture.
    unsigned addTexture(TexSharedPtr texture) {
        mVec.push_back(std::move(texture));
        return static_cast<unsigned>(mVec.size() - 1);
    }

    void removeTexture(unsigned textureIndex) {
        if (textureIndex < mVec.size()) {
            mVec.erase(mVec.begin() + textureIndex);
        }
    }

    unsigned getTextureCount() const { return static_cast<unsigned>(mVec.size()); }

    std::vector<TexSharedPtr>& getVector() { return mVec; }
};


#endif // TEXTURE_GROUP_HPP
