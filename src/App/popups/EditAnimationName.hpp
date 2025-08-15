#ifndef POPUP_EDITANIMATIONNAME_HPP
#define POPUP_EDITANIMATIONNAME_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class EditAnimationName : public Popup, public Singleton<EditAnimationName> {
    friend class Singleton<EditAnimationName>;
public:
    void update();
    void setAnimIndex(int index) {
        mAnimationIndex = index;
    }
protected:
    const char* strId() {
        return "###EditAnimationName";
    }
private:
    EditAnimationName() = default;
    int mAnimationIndex = -1;
};

}

#endif // POPUP_EDITANIMATIONNAME_HPP
