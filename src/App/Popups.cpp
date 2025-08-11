#include <array>

#include "Popups.hpp"

#include "popups/Popup.hpp"
#include "popups/SheetRepackFailed.hpp"
#include "popups/EditAnimationName.hpp"

namespace Popups {

int  _swapAnimationIdx { -1 };

int _oldTextureSizeX { -1 };
int _oldTextureSizeY { -1 };

int _editPartNameArrangeIdx { -1 };
int _editPartNamePartIdx { -1 };


static std::array<Popup*, 2> popups;

void createSingletons() {
    popups = {
        static_cast<Popup*>(&EditAnimationName::createSingleton()),

        static_cast<Popup*>(&SheetRepackFailed::createSingleton()),
    };
}

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

    Popup_SwapAnimation(_swapAnimationIdx);

    Popup_EditPartName(_editPartNameArrangeIdx, _editPartNamePartIdx);

    Popup_SpritesheetManager();

    END_GLOBAL_POPUP();
}
