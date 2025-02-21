#ifndef TEXTUREGROUP_HPP
#define TEXTUREGROUP_HPP

#include <vector>

#include <memory>

#include "Texture.hpp"

class TextureGroup {
private:
    std::vector<std::shared_ptr<Texture>> group;
    unsigned baseTextureIndex { 0 };

    bool varyingEnabled { false };
public:
    TextureGroup() = default;
    ~TextureGroup() = default;

    bool getVaryingEnabled() const { return this->varyingEnabled; }
    void setVaryingEnabled(bool enabled) { this->varyingEnabled = enabled; } 

    std::shared_ptr<Texture>& getBaseTexture();

    unsigned getBaseTextureIndex() const { return this->baseTextureIndex; }
    void setBaseTextureIndex(unsigned index) { this->baseTextureIndex = index; }

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

    unsigned getTextureCount() const { return this->group.size(); }

    std::vector<std::shared_ptr<Texture>>& getVector() { return this->group; };
};


#endif // TEXTUREGROUP_HPP
