#ifndef POPUP_MINTERPOLATEKEYS_HPP
#define POPUP_MINTERPOLATEKEYS_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class MInterpolateKeys : public Popup, public Singleton<MInterpolateKeys> {
    friend class Singleton<MInterpolateKeys>;
public:
    void update();
protected:
    const char* strId() {
        return "MInterpolateKeys";
    }
private:
    MInterpolateKeys() = default;
};

}

#endif // POPUP_MINTERPOLATEKEYS_HPP
