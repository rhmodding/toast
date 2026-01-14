#include "PopupHandler.hpp"

#include <array>

#include <imgui.h>
#include <imgui_internal.h>

#include "popups/Popup.hpp"
#include "popups/AllPopups.hpp"

#include "manager/AppState.hpp"

static std::array<Popup *, 13> sPopups;
static bool sPopupsInitialized = false;

void Popups::createSingletons() {
    if (sPopupsInitialized) {
        return;
    }

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
    if (!sPopupsInitialized) {
        return;
    }

    for (size_t i = 0; i < sPopups.size(); i++) {
        sPopups[i] = nullptr;
    }

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
        throw std::runtime_error("Popups::update: popups not (yet) initialized!");
    }

    ImGui::PushOverrideID(GLOBAL_POPUP_ID);

    for (Popup *p : sPopups) {
        p->update();
    }

    ImGui::PopID();
}
