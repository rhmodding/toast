#ifndef EASE_UTIL_HPP
#define EASE_UTIL_HPP

namespace EaseUtil {

static inline float InOut(float t) {
    return t < .5f ? 2.f * t * t : -1.f + (4.f - 2.f * t) * t;
}
static inline float In(float t) {
    return t * t;
}
static inline float Out(float t) {
    return 1.f - (1.f - t) * (1.f - t);
}

} // namespace EaseUtil

#endif // EASE_UTIL_HPP
