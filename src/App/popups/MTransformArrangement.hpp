#ifndef POPUP_MTRANSFORMARRANGEMENT_HPP
#define POPUP_MTRANSFORMARRANGEMENT_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class MTransformArrangement: public Popup, public Singleton<MTransformArrangement> {
    friend class Singleton<MTransformArrangement>;
public:
    void update();
protected:
    const char* strId() {
        return "MTransformArrangement";
    }
private:
    MTransformArrangement() = default;
};

}

#endif // POPUP_MTRANSFORMARRANGEMENT_HPP
