#ifndef POPUP_MOPTIMIZEGLOBAL_HPP
#define POPUP_MOPTIMIZEGLOBAL_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class MOptimizeGlobal : public Popup, public Singleton<MOptimizeGlobal> {
    friend class Singleton<MOptimizeGlobal>;
public:
    void update();
protected:
    const char* strId() {
        return "###MOptimizeGlobal";
    }
private:
    MOptimizeGlobal() = default;
};

}

#endif // POPUP_MOPTIMIZEGLOBAL_HPP
