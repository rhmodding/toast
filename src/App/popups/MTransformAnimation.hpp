#ifndef POPUP_MTRANSFORMANIMATION_HPP
#define POPUP_MTRANSFORMANIMATION_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class MTransformAnimation: public Popup, public Singleton<MTransformAnimation> {
    friend class Singleton<MTransformAnimation>;
public:
    void Update();
private:
    MTransformAnimation() = default;
};

}

#endif // POPUP_MTRANSFORMANIMATION_HPP
