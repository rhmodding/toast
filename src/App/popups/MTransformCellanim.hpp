#ifndef POPUP_MTRANSFORMCELLANIM_HPP
#define POPUP_MTRANSFORMCELLANIM_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class MTransformCellanim: public Popup, public Singleton<MTransformCellanim> {
    friend class Singleton<MTransformCellanim>;
public:
    void update();
protected:
    const char* strId() {
        return "MTransformCellanim";
    }
private:
    MTransformCellanim() = default;
};

}

#endif // POPUP_MTRANSFORMCELLANIM_HPP
