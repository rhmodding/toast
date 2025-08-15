#include "PopupHandler.hpp"

#include <array>

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
#include "popups/MTransformAnimation.hpp"
#include "popups/MTransformArrangement.hpp"
#include "popups/MTransformCellanim.hpp"

#include "manager/AppState.hpp"

static std::array<Popup*, 13> popups;

void Popups::createSingletons() {
    popups = {
        &EditAnimationName::createSingleton(),
        &SheetRepackFailed::createSingleton(),
        &EditPartName::createSingleton(),
        &MTransformCellanim::createSingleton(),
        &MTransformAnimation::createSingleton(),
        &MTransformArrangement::createSingleton(),
        &MPadRegion::createSingleton(),
        &MOptimizeGlobal::createSingleton(),
        &MInterpolateKeys::createSingleton(),
        &WaitForModifiedTexture::createSingleton(),
        &ModifiedTextureSize::createSingleton(),
        &SwapAnimation::createSingleton(),
        &SpritesheetManager::createSingleton(),
    };
}

void Popups::update() {
    BEGIN_GLOBAL_POPUP();

    for (Popup* p : popups) {
        p->Update();
    }

    END_GLOBAL_POPUP();
}
