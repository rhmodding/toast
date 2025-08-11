#include <list>

#include "Popups.hpp"

#include "popups/Popup.hpp"
#include "popups/SheetRepackFailed.hpp"

namespace Popups {

int  _editAnimationNameIdx { -1 };
int  _swapAnimationIdx { -1 };

int _oldTextureSizeX { -1 };
int _oldTextureSizeY { -1 };

int _editPartNameArrangeIdx { -1 };
int _editPartNamePartIdx { -1 };

static SheetRepackFailed sheetRepackFailed = SheetRepackFailed();

static std::list<Popup*> popups = {
    static_cast<Popup*>(&sheetRepackFailed)
};

} // namespace Popups

#include "manager/AppState.hpp"

#include "popups/Popup_MTransformCellanim.hpp"
#include "popups/Popup_MTransformAnimation.hpp"
#include "popups/Popup_MTransformArrangement.hpp"
#include "popups/Popup_MPadRegion.hpp"
#include "popups/Popup_MOptimizeGlobal.hpp"
#include "popups/Popup_MInterpolateKeys.hpp"

#include "popups/Popup_ModifiedTextureSize.hpp"

#include "popups/Popup_WaitForModifiedTexture.hpp"

#include "popups/Popup_EditAnimationName.hpp"
#include "popups/Popup_SwapAnimation.hpp"

#include "popups/Popup_EditPartName.hpp"

#include "popups/Popup_SpritesheetManager.hpp"

void Popups::Update() {
    BEGIN_GLOBAL_POPUP();

    for (Popup* p: popups) {
        p->Update();
    }

    Popup_MTransformCellanim();
    Popup_MTransformAnimation();
    Popup_MTransformArrangement();
    Popup_MPadRegion();
    Popup_MOptimizeGlobal();
    Popup_MInterpolateKeys();

    Popup_ModifiedTextureSize(_oldTextureSizeX, _oldTextureSizeY);

    Popup_WaitForModifiedTexture();

    Popup_EditAnimationName(_editAnimationNameIdx);
    Popup_SwapAnimation(_swapAnimationIdx);

    Popup_EditPartName(_editPartNameArrangeIdx, _editPartNamePartIdx);

    Popup_SpritesheetManager();

    END_GLOBAL_POPUP();
}
