#ifndef POPUP_SHEETREPACKFAILED_HPP
#define POPUP_SHEETREPACKFAILED_HPP

#include "manager/Singleton.hpp"

#include "Popup.hpp"

namespace Popups {
    class SheetRepackFailed : public Popup, public Singleton<SheetRepackFailed> {
        friend class Singleton<SheetRepackFailed>;
    public:
        virtual void Update();
    private:
        SheetRepackFailed() = default;
    };
};

#endif // POPUP_SHEETREPACKFAILED_HPP
