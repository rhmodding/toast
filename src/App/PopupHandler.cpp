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

static std::array<Popup*, 13> sPopups;
static bool sPopupsInitialized = false;

void Popups::createSingletons() {
    if (sPopupsInitialized)
        return;
    sPopups = {
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
    sPopupsInitialized = true;
}

void Popups::destroySingletons() {
    if (!sPopupsInitialized)
        return;
    for (int i = 0; i < sPopups.size(); i++)
        sPopups[i] = nullptr;

    EditAnimationName::destroySingleton();
    SheetRepackFailed::destroySingleton();
    EditPartName::destroySingleton();
    MTransformCellanim::destroySingleton();
    MTransformAnimation::destroySingleton();
    MTransformArrangement::destroySingleton();
    MPadRegion::destroySingleton();
    MOptimizeGlobal::destroySingleton();
    MInterpolateKeys::destroySingleton();
    WaitForModifiedTexture::destroySingleton();
    ModifiedTextureSize::destroySingleton();
    SwapAnimation::destroySingleton();
    SpritesheetManager::destroySingleton();

    sPopupsInitialized = false;
}

void Popups::update() {
    if (!sPopupsInitialized) {
        printf("Tried to update popups while they were not initialized - what\n");
        return;
    }

    ImGui::PushOverrideID(GLOBAL_POPUP_ID);

    for (Popup* p : sPopups) {
        p->Update();
    }

    ImGui::PopID();
}
