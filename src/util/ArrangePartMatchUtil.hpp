#ifndef ARRANGE_PART_MATCH_UTIL_HPP
#define ARRANGE_PART_MATCH_UTIL_HPP

#include "cellanim/CellAnim.hpp"

#include <cstddef>

namespace ArrangePartMatchUtil {

int tryMatchByName(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
);

int tryMatchById(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
);

int tryMatchByCell(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
);

inline int tryMatch(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement, int partIndex
) {
    int matcher = tryMatchByName(part, arrangement, partIndex);
    if (matcher < 0) {
        matcher = tryMatchById(part, arrangement, partIndex);
    }
    if (matcher < 0) {
        matcher = tryMatchByCell(part, arrangement, partIndex);
    }

    return (matcher >= 0) ? matcher : -1;
}


} // namespace ArrangePartMatchUtil

#endif // ARRANGE_PART_MATCH_UTIL_HPP
