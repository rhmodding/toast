#include <array>

#include "Popups.hpp"

#include "popups/Popup.hpp"
#include "popups/SheetRepackFailed.hpp"
#include "popups/EditAnimationName.hpp"
#include "popups/EditPartName.hpp"
#include "popups/SpritesheetManager.hpp"

namespace Popups {

int  _swapAnimationIdx { -1 };

int _oldTextureSizeX { -1 };
int _oldTextureSizeY { -1 };


static std::array<Popup*, 4> popups;

void createSingletons() {
    popups = {
        static_cast<Popup*>(&EditAnimationName::createSingleton()),
        static_cast<Popup*>(&SheetRepackFailed::createSingleton()),
        static_cast<Popup*>(&EditPartName::createSingleton()),
        static_cast<Popup*>(&SpritesheetManager::createSingleton()),
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

    END_GLOBAL_POPUP();
}
