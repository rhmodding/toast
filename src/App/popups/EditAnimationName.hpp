#ifndef POPUP_EDITANIMATIONNAME_HPP
#define POPUP_EDITANIMATIONNAME_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {
    class EditAnimationName : public Popup, public Singleton<EditAnimationName> {
        friend class Singleton<EditAnimationName>;
    public:
        virtual void Update();
        void setAnimIndex(int index) {
            mAnimationIndex = index;
        }
    private:
        EditAnimationName() = default;
        int mAnimationIndex = -1;
    };
}

#endif // POPUP_EDITANIMATIONNAME_HPP
