#ifndef POPUP_MODIFIEDTEXTURESIZE_HPP
#define POPUP_MODIFIEDTEXTURESIZE_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class ModifiedTextureSize: public Popup, public Singleton<ModifiedTextureSize> {
    friend class Singleton<ModifiedTextureSize>;
public:
    void update();
    void setOldTextureSizes(int oldSizeX, int oldSizeY) {
        mOldTextureSizeX = oldSizeX;
        mOldTextureSizeY = oldSizeY;
    }
protected:
    const char* strId() {
        return "###ModifiedTextureSize";
    }
private:
    ModifiedTextureSize() = default;
    int mOldTextureSizeX = -1;
    int mOldTextureSizeY = -1;
};

}

#endif // POPUP_MODIFIEDTEXTURESIZE_HPP
