#ifndef POPUP_EDITPARTNAME_HPP
#define POPUP_EDITPARTNAME_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {

class EditPartName : public Popup, public Singleton<EditPartName> {
    friend class Singleton<EditPartName>;
public:
    void update();
    void setArrangementIndex(int index) {
        mArrangementIndex = index;
    }
    void setPartIndex(int index) {
        mPartIndex = index;
    }
    void setIndices(int arrIndex, int pIndex) {
        mArrangementIndex = arrIndex;
        mPartIndex = pIndex;
    }
protected:
    const char* strId() {
        return "###EditPartName";
    }
private:
    EditPartName() = default;
    int mArrangementIndex = -1;
    int mPartIndex = -1;
};

}

#endif // POPUP_EDITPARTNAME_HPP
