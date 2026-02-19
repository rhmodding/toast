#ifndef POPUP_NEWWIZARD_HPP
#define POPUP_NEWWIZARD_HPP

#include "manager/Singleton.hpp"

#include "Popup.hpp"

namespace Popups {

class NewWizard : public Popup, public Singleton<NewWizard> {
    friend class Singleton<NewWizard>;
private:
    NewWizard() {
        resetFields();
    }
public:
    void update();
protected:
    const char* strId() {
        return "###NewWizard";
    }

private:
    void resetFields() {
        mCellAnimAdd.clear();
    }

private:
    struct CellAnimEnt {
        std::string name;
    };

    enum {
        TARGET_RVL,
        TARGET_CTR
    };

private:
    std::vector<CellAnimEnt> mCellAnimAdd;
    CellAnimEnt mNewCellAnimEnt;
    int mSelectedTarget; // TARGET_RVL or TARGET_CTR
};

};

#endif // POPUP_NEWWIZARD_HPP
