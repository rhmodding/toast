#include "Popups.hpp"

#include "../AppState.hpp"

#include "popups/Popup_MTransformArrangement.hpp"
#include "popups/Popup_MPadRegion.hpp"
#include "popups/Popup_MOptimizeGlobal.hpp"

#include "popups/Popup_SessionErr.hpp"
#include "popups/Popup_TextureExportFailed.hpp"

#include "popups/Popup_ModifiedTextureSize.hpp"

#include "popups/Popup_WaitForModifiedTexture.hpp"

#include "popups/Popup_ExitWithChanges.hpp"

#include "popups/Popup_EditorDataExpected.hpp"

#include "popups/Popup_CloseModifiedSession.hpp"

void Popups::Update() {
    BEGIN_GLOBAL_POPUP;

    Popup_MTransformArrangement();
    Popup_MPadRegion();
    Popup_MOptimizeGlobal();

    Popup_SessionErr();
    Popup_TextureExportFailed();

    Popup_ModifiedTextureSize();

    Popup_WaitForModifiedTexture();

    if (_openExitWithChangesPopup) {
        AppState::getInstance().OpenGlobalPopup("###ExitWithChanges");
        _openExitWithChangesPopup = false;
    }

    Popup_ExitWithChanges();

    Popup_EditorDataExpected();

    Popup_CloseModifiedSession();

    END_GLOBAL_POPUP;
}