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

namespace Popups {

    static std::array<Popup*, 7> popups;

    void createSingletons() {
        popups = {
            static_cast<Popup*>(&EditAnimationName::createSingleton()),
            static_cast<Popup*>(&SheetRepackFailed::createSingleton()),
            static_cast<Popup*>(&EditPartName::createSingleton()),
            static_cast<Popup*>(&WaitForModifiedTexture::createSingleton()),
            static_cast<Popup*>(&ModifiedTextureSize::createSingleton()),
            static_cast<Popup*>(&SwapAnimation::createSingleton()),
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
    Popup_MInterpolateKeys();

    END_GLOBAL_POPUP();
}
