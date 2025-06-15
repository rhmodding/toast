#ifndef BEZIER_UTIL_HPP
#define BEZIER_UTIL_HPP

#include <imgui.h>

namespace BezierUtil {

// Newton-Raphson
// Results in a tiny X error (usually not more than about +- 0.0000006 on 10 iterations).
ImVec2 Approx(const float x, const float P[4], unsigned maxIterations = 10);

static inline float ApproxY(const float x, const float P[4], unsigned maxIterations = 10) {
    return Approx(x, P, maxIterations).y;
}

bool ImWidget(const char* label, float P[4], bool handlesEnabled = true);

} // namespace BezierUtil

#endif // BEZIER_UTIL_HPP
