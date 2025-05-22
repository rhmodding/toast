#ifndef TEXTURE_GROUP_HPP
#define TEXTURE_GROUP_HPP

#include <vector>

#include <memory>

#include "Texture.hpp"

class TextureGroup {
private:
    std::vector<std::shared_ptr<Texture>> mVec;
    unsigned mBaseTextureIndex { 0 };

    bool mVaryingEnabled { false };
public:
    TextureGroup() = default;
    ~TextureGroup() = default;

    bool getVaryingEnabled() const { return mVaryingEnabled; }
    void setVaryingEnabled(bool enabled) { mVaryingEnabled = enabled; } 

    std::shared_ptr<Texture>& getBaseTexture();

    unsigned getBaseTextureIndex() const { return mBaseTextureIndex; }
    void setBaseTextureIndex(unsigned index) { mBaseTextureIndex = index; }

    // Gets texture at index baseTextureIndex + varying.
    // Note A: if varying is disabled, this will return the base texture.
    // Note B: out of bounds values will wrap around.
    std::shared_ptr<Texture>& getTextureByVarying(unsigned varying);

    // Gets texture at specified index.
    // Note: out of bounds values will wrap around.
    std::shared_ptr<Texture>& getTextureByIndex(unsigned index);

    // Add a texture to the group.
    //
    // Returns: the index of the new texture.
    unsigned addTexture(std::shared_ptr<Texture> texture);

    void removeTexture(unsigned textureIndex);

    unsigned getTextureCount() const { return mVec.size(); }

    std::vector<std::shared_ptr<Texture>>& getVector() { return mVec; };
};


#endif // TEXTURE_GROUP_HPP
