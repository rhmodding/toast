#include "AnimLabelHeaderUtil.hpp"

std::string AnimLabelHeaderUtil::build(const CellAnim::CellAnimObject &cellAnim) {
    std::ostringstream stream;
    for (size_t i = 0; i < cellAnim.getAnimations().size(); i++) {
        const auto& animation = cellAnim.getAnimation(i);
        if (animation.name.empty()) {
            continue;
        }

        stream <<
            "#define " << cellAnim.getName() << '_' << animation.name << '\t' << std::to_string(i) <<
            "\t// " << (animation.comment.empty() ? "(null)" : animation.comment) << "\r\n";
    }

    return stream.str();
}
