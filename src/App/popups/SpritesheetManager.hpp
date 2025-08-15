#ifndef POPUP_SPRITESHEETMANAGER_HPP
#define POPUP_SPRITESHEETMANAGER_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class SpritesheetManager: public Popup, public Singleton<SpritesheetManager> {
    friend class Singleton<SpritesheetManager>;
public:
    void Update();
private:
    SpritesheetManager() = default;
};

}

#endif // POPUP_SPRITESHEETMANAGER_HPP
