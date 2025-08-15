#ifndef POPUP_MPADREGION_HPP
#define POPUP_MPADREGION_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class MPadRegion : public Popup, public Singleton<MPadRegion> {
    friend class Singleton<MPadRegion>;
public:
    void update();
protected:
    const char* strId() {
        return "MPadRegion";
    }
private:
    MPadRegion() = default;
};

}

#endif // POPUP_MPADREGION_HPP
