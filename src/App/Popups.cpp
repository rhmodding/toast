#include "Popups.hpp"

namespace Popups {

bool _openExitWithChangesPopup { false };

int  _editAnimationNameIdx { -1 };
int  _swapAnimationIdx { -1 };

} // namespace Popups

#include "../AppState.hpp"

#include "popups/Popup_MTransformArrangement.hpp"
#include "popups/Popup_MPadRegion.hpp"
#include "popups/Popup_MOptimizeGlobal.hpp"
#include "popups/Popup_MInterpolateKeys.hpp"

#include "popups/Popup_SessionErr.hpp"
#include "popups/Popup_TextureExportFailed.hpp"

#include "popups/Popup_ModifiedTextureSize.hpp"

#include "popups/Popup_WaitForModifiedTexture.hpp"

#include "popups/Popup_ExitWithChanges.hpp"

#include "popups/Popup_CloseModifiedSession.hpp"

#include "popups/Popup_EditAnimationName.hpp"
#include "popups/Popup_SwapAnimation.hpp"

void Popups::Update() {
    BEGIN_GLOBAL_POPUP;

    Popup_MTransformArrangement();
    Popup_MPadRegion();
    Popup_MOptimizeGlobal();
    Popup_MInterpolateKeys();

    Popup_SessionErr();
    Popup_TextureExportFailed();

    Popup_ModifiedTextureSize();

    Popup_WaitForModifiedTexture();

    if (_openExitWithChangesPopup) {
        AppState::getInstance().OpenGlobalPopup("###ExitWithChanges");
        _openExitWithChangesPopup = false;
    }

    Popup_ExitWithChanges();

    Popup_CloseModifiedSession();

    Popup_EditAnimationName(_editAnimationNameIdx);
    Popup_SwapAnimation(_swapAnimationIdx);

    END_GLOBAL_POPUP;
}
