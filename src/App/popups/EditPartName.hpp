#ifndef POPUP_EDITPARTNAME_HPP
#define POPUP_EDITPARTNAME_HPP

#include "Popup.hpp"

#include "manager/Singleton.hpp"

namespace Popups {
    class EditPartName : public Popup, public Singleton<EditPartName> {
        friend class Singleton<EditPartName>;
        public:
            void Update();
            void setArrangementIndex(int index) {
                arrangementIndex = index;
            }
            void setPartIndex(int index) {
                partIndex = index;
            }
            void setIndices(int arrIndex, int pIndex) {
                arrangementIndex = arrIndex;
                partIndex = pIndex;
            }
        private:
            EditPartName() = default;
            int arrangementIndex = -1;
            int partIndex = -1;
    };
}

#endif // POPUP_EDITPARTNAME_HPP
