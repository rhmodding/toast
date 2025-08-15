#ifndef POPUP_SWAPANIMATION_HPP
#define POPUP_SWAPANIMATION_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class SwapAnimation : public Popup, public Singleton<SwapAnimation> {
    friend class Singleton<SwapAnimation>;
public:
    void Update();
    void setAnimationIndex(int index) {
        mAnimationIndex = index;
    }
private:
    SwapAnimation() = default;
    int mAnimationIndex = -1;
};

}

#endif // POPUP_SWAPANIMATION_HPP
