#ifndef POPUP_SPRITESHEETMANAGER_HPP
#define POPUP_SPRITESHEETMANAGER_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class SpritesheetManager: public Popup, public Singleton<SpritesheetManager> {
    friend class Singleton<SpritesheetManager>;
public:
    void update();
protected:
    const char* strId() {
        return "###SpritesheetManager";
    }
private:
    SpritesheetManager() = default;
};

}

#endif // POPUP_SPRITESHEETMANAGER_HPP
