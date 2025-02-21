#include "TextureGroup.hpp"

std::shared_ptr<Texture>& TextureGroup::getBaseTexture() {
    return this->group[this->baseTextureIndex % this->group.size()];
}

std::shared_ptr<Texture>& TextureGroup::getTextureByVarying(unsigned varying) {
    unsigned index = this->baseTextureIndex;
    if (this->varyingEnabled)
        index += varying;

    return this->group[index % this->group.size()];
}

std::shared_ptr<Texture>& TextureGroup::getTextureByIndex(unsigned index) {
    return this->group[index % this->group.size()];
}

unsigned TextureGroup::addTexture(std::shared_ptr<Texture> texture) {
    this->group.push_back(texture);
    return this->group.size() - 1;
}

void TextureGroup::removeTexture(unsigned textureIndex) {
    if (textureIndex >= this->group.size())
        return;

    auto textureIt = this->group.begin() + textureIndex;
    this->group.erase(textureIt);
}
