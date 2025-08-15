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
#include "popups/MOptimizeGlobal.hpp"
#include "popups/MPadRegion.hpp"

namespace Popups {

static std::array<Popup*, 10> popups;

void createSingletons() {
    popups = {
        &EditAnimationName::createSingleton(),
        &SheetRepackFailed::createSingleton(),
        &EditPartName::createSingleton(),
        &MPadRegion::createSingleton(),
        &MOptimizeGlobal::createSingleton(),
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

void Popups::Update() {
    BEGIN_GLOBAL_POPUP();

    for (Popup* p : popups) {
        p->Update();
    }

    Popup_MTransformCellanim();
    Popup_MTransformAnimation();
    Popup_MTransformArrangement();

    END_GLOBAL_POPUP();
}
