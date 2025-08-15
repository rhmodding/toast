#ifndef POPUP_SHEETREPACKFAILED_HPP
#define POPUP_SHEETREPACKFAILED_HPP

#include "manager/Singleton.hpp"

#include "Popup.hpp"

namespace Popups {

class SheetRepackFailed : public Popup, public Singleton<SheetRepackFailed> {
    friend class Singleton<SheetRepackFailed>;
public:
    void update();
protected:
    const char* strId() {
        return "###SheetRepackFailed";
    }
private:
    SheetRepackFailed() = default;
};

};

#endif // POPUP_SHEETREPACKFAILED_HPP
