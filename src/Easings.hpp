#ifndef EASINGS_HPP
#define EASINGS_HPP

namespace Easings {

inline float InOut(float t) {
    return t < .5f ? 2.f * t * t : -1.f + (4.f - 2.f * t) * t;
}
inline float In(float t) {
    return t * t;
}
inline float Out(float t) {
    return 1.f - (1.f - t) * (1.f - t);
}

} // namespace Easings

#endif // EASINGS_HPP
