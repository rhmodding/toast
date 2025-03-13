#include "Popups.hpp"

namespace Popups {

bool _openExitWithChangesPopup { false };

int  _editAnimationNameIdx { -1 };
int  _swapAnimationIdx { -1 };

int _oldTextureSizeX { -1 };
int _oldTextureSizeY { -1 };

int _deleteAnimationIdx { -1 };

int _editPartNameArrangeIdx { -1 };
int _editPartNamePartIdx { -1 };

} // namespace Popups

#include "../AppState.hpp"

#include "popups/Popup_MTransformCellanim.hpp"
#include "popups/Popup_MTransformArrangement.hpp"
#include "popups/Popup_MPadRegion.hpp"
#include "popups/Popup_MOptimizeGlobal.hpp"
#include "popups/Popup_MInterpolateKeys.hpp"

#include "popups/Popup_SessionErr.hpp"

#include "popups/Popup_TextureExportFailed.hpp"
#include "popups/Popup_SheetRepackFailed.hpp"

#include "popups/Popup_ModifiedTextureSize.hpp"

#include "popups/Popup_WaitForModifiedTexture.hpp"

#include "popups/Popup_DeleteAnimation.hpp"

#include "popups/Popup_ExitWithChanges.hpp"

#include "popups/Popup_CloseModifiedSession.hpp"

#include "popups/Popup_EditAnimationName.hpp"
#include "popups/Popup_SwapAnimation.hpp"

#include "popups/Popup_EditPartName.hpp"

#include "popups/Popup_SpritesheetManager.hpp"

void Popups::Update() {
    BEGIN_GLOBAL_POPUP();

    Popup_MTransformCellanim();
    Popup_MTransformArrangement();
    Popup_MPadRegion();
    Popup_MOptimizeGlobal();
    Popup_MInterpolateKeys();

    Popup_SessionErr();

    Popup_TextureExportFailed();
    Popup_SheetRepackFailed();

    Popup_ModifiedTextureSize(_oldTextureSizeX, _oldTextureSizeY);

    Popup_WaitForModifiedTexture();

    Popup_DeleteAnimation(_deleteAnimationIdx);

    if (_openExitWithChangesPopup) {
        OPEN_GLOBAL_POPUP("###ExitWithChanges");
        _openExitWithChangesPopup = false;
    }

    Popup_ExitWithChanges();

    Popup_CloseModifiedSession();

    Popup_EditAnimationName(_editAnimationNameIdx);
    Popup_SwapAnimation(_swapAnimationIdx);

    Popup_EditPartName(_editPartNameArrangeIdx, _editPartNamePartIdx);

    Popup_SpritesheetManager();

    END_GLOBAL_POPUP();
}
