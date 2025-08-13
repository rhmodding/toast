#ifndef POPUP_MODIFIEDTEXTURESIZE_HPP
#define POPUP_MODIFIEDTEXTURESIZE_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {
    class ModifiedTextureSize: public Popup, public Singleton<ModifiedTextureSize> {
        friend class Singleton<ModifiedTextureSize>;
        public:
            void Update();
            void setOldTextureSizes(int oldSizeX, int oldSizeY) {
                oldTextureSizeX = oldSizeX;
                oldTextureSizeY = oldSizeY;
            }
        private:
            ModifiedTextureSize() = default;
            int oldTextureSizeX = -1;
            int oldTextureSizeY = -1;
    };
}

#endif // POPUP_MODIFIEDTEXTURESIZE_HPP
