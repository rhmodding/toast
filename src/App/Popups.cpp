#include <array>

#include "Popups.hpp"

#include "popups/Popup.hpp"
#include "popups/SheetRepackFailed.hpp"
#include "popups/EditAnimationName.hpp"
#include "popups/EditPartName.hpp"
#include "popups/SpritesheetManager.hpp"
#include "popups/SwapAnimation.hpp"
#include "popups/WaitForModifiedTexture.hpp"
#include "popups/ModifiedTextureSize.hpp"
#include "popups/MInterpolateKeys.hpp"

namespace Popups {

static std::array<Popup*, 8> popups;

void createSingletons() {
    popups = {
        &EditAnimationName::createSingleton(),
        &SheetRepackFailed::createSingleton(),
        &EditPartName::createSingleton(),
        &MInterpolateKeys::createSingleton(),
        &WaitForModifiedTexture::createSingleton(),
        &ModifiedTextureSize::createSingleton(),
        &SwapAnimation::createSingleton(),
        &SpritesheetManager::createSingleton(),
    };
}

} // namespace Popups

#include "manager/AppState.hpp"

#include "popups/Popup_MTransformCellanim.hpp"
#include "popups/Popup_MTransformAnimation.hpp"
#include "popups/Popup_MTransformArrangement.hpp"
#include "popups/Popup_MPadRegion.hpp"
#include "popups/Popup_MOptimizeGlobal.hpp"

void Popups::Update() {
    BEGIN_GLOBAL_POPUP();

    for (Popup* p : popups) {
        p->Update();
    }

    Popup_MTransformCellanim();
    Popup_MTransformAnimation();
    Popup_MTransformArrangement();
    Popup_MPadRegion();
    Popup_MOptimizeGlobal();

    END_GLOBAL_POPUP();
}
