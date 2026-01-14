#ifndef TWEEN_ANIM_UTIL_HPP
#define TWEEN_ANIM_UTIL_HPP

#include "cellanim/CellAnim.hpp"

#include <vector>

namespace TweenAnimUtil {

CellAnim::AnimationKey tweenAnimKeys(
    std::vector<CellAnim::Arrangement>& arrangements, bool dontTweenArrangement,
    const CellAnim::AnimationKey& k0, const CellAnim::AnimationKey& k1, float t
);

} // namespace TweenAnimUtil

#endif // TWEEN_ANIM_UTIL_HPP
