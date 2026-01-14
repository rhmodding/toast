#include "TweenAnimUtil.hpp"

#include "ArrangePartMatchUtil.hpp"

CellAnim::AnimationKey TweenAnimUtil::tweenAnimKeys(
    std::vector<CellAnim::Arrangement>& arrangements, bool dontTweenArrangement,
    const CellAnim::AnimationKey& k0, const CellAnim::AnimationKey& k1, float t
) {
    CellAnim::AnimationKey newKey = k0;

    newKey.transform = newKey.transform.lerp(k1.transform, t);
    newKey.opacity = static_cast<uint8_t>(std::clamp<int>(
        LERP_INTS(k0.opacity, k1.opacity, t),
        0x00, 0xFF
    ));

    newKey.foreColor = newKey.foreColor.lerp(k1.foreColor, t);
    newKey.backColor = newKey.backColor.lerp(k1.backColor, t);

    // Create new interpolated arrangement if not equal
    if ((!dontTweenArrangement) && k0.arrangementIndex != k1.arrangementIndex) {
        const CellAnim::Arrangement& endArrangement = arrangements.at(k1.arrangementIndex);
        
        CellAnim::Arrangement newArrangement;
        newArrangement.parts = arrangements.at(k0.arrangementIndex).parts;

        for (unsigned j = 0; j < newArrangement.parts.size(); j++) {
            auto& part = newArrangement.parts[j];

            int endPartIndex = ArrangePartMatchUtil::tryMatch(part, endArrangement, j);
            if (endPartIndex < 0)
                endPartIndex = std::min<int>(j, endArrangement.parts.size() - 1);
            
            const CellAnim::ArrangementPart& endPart = endArrangement.parts[endPartIndex];

            part.transform = part.transform.lerp(endPart.transform, t);

            // Flip non-tweenable properties over once T is over half
            if (t > .5f) {
                part.flipX = endPart.flipX;
                part.flipY = endPart.flipY;

                part.cellOrigin = endPart.cellOrigin;
                part.cellSize = endPart.cellSize;

                part.editorVisible = endPart.editorVisible;
                part.editorLocked = endPart.editorLocked;
            }

            part.opacity = std::clamp<int>(
                LERP_INTS(part.opacity, endPart.opacity, t),
                0x00, 0xFF
            );

            part.foreColor = part.foreColor.lerp(endPart.foreColor, t);
            part.backColor = part.backColor.lerp(endPart.backColor, t);
        }

        newKey.arrangementIndex = arrangements.size();
        arrangements.push_back(newArrangement);
    }

    return newKey;
}
