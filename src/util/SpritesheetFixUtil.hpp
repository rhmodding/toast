#ifndef SPRITESHEET_FIX_UTIL
#define SPRITESHEET_FIX_UTIL

#include "manager/SessionManager.hpp"

namespace SpritesheetFixUtil {

// Repack the sheet cels into a (hopefully) more efficient configuration.
// Returns true if succeeded, false if failed.
bool FixRepack(Session& session, int sheetIndex = -1 /* Use current sheet */);

// Fix alpha bleeding issues that cause dark fringing where transparent and opaque meet.
bool FixAlphaBleed(Session& session, int sheetIndex = -1 /* Use current sheet */);

} // namespace SpritesheetFixUtil

#endif // SPRITESHEET_FIX_UTIL
