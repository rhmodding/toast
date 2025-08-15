#ifndef POPUP_WAITFORMODIFIEDTEXTURE_HPP
#define POPUP_WAITFORMODIFIEDTEXTURE_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class WaitForModifiedTexture: public Popup, public Singleton<WaitForModifiedTexture> {
    friend class Singleton<WaitForModifiedTexture>;
public:
    void Update();
private:
    WaitForModifiedTexture() = default;
};

}

#endif // POPUP_WAITFORMODIFIEDTEXTURE_HPP
