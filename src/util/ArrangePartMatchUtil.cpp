#include "ArrangePartMatchUtil.hpp"

int ArrangePartMatchUtil::tryMatchByName(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
) {
    if (part.editorName.empty())
        return -1;

    int closestIndex = -1;
    int closestDistance = std::numeric_limits<int>::max();

    for (int i = 0; i < static_cast<int>(arrangement.parts.size()); i++) {
        const auto& lPart = arrangement.parts[i];
        if (lPart.editorName == part.editorName) {
            int distance = std::abs(static_cast<int>(i) - partIndex);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestIndex = static_cast<int>(i);
            }
        }
    }

    return closestIndex;
}

int ArrangePartMatchUtil::tryMatchById(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
) {
    if (part.id == 0)
        return -1;

    int closestIndex = -1;
    int closestDistance = std::numeric_limits<int>::max();

    for (int i = 0; i < static_cast<int>(arrangement.parts.size()); i++) {
        const auto& lPart = arrangement.parts[i];
        if (lPart.id == part.id) {
            int distance = std::abs(static_cast<int>(i) - partIndex);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestIndex = static_cast<int>(i);
            }
        }
    }

    return closestIndex;
}

int ArrangePartMatchUtil::tryMatchByCell(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
) {
    int closestIndex = -1;
    int closestDistance = std::numeric_limits<int>::max();

    for (int i = 0; i < static_cast<int>(arrangement.parts.size()); i++) {
        const auto& lPart = arrangement.parts[i];
        if (
            lPart.cellOrigin == part.cellOrigin &&
            lPart.cellSize == part.cellSize
        ) {
            int distance = std::abs(static_cast<int>(i) - partIndex);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestIndex = static_cast<int>(i);
            }
        }
    }

    return closestIndex;
}
