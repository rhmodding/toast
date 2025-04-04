#ifndef ONIONSKINSTATE_HPP
#define ONIONSKINSTATE_HPP

#include <cstdint>

struct OnionSkinState {
    bool enabled { false };
    bool drawUnder { false };
    bool rollOver { false };

    unsigned backCount { 3 };
    unsigned frontCount { 2 };

    uint8_t opacity { 50 };
};

#endif // ONIONSKINSTATE_HPP
