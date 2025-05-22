#ifndef ONION_SKIN_STATE_HPP
#define ONION_SKIN_STATE_HPP

#include <cstdint>

struct OnionSkinState {
    bool enabled { false };
    bool drawUnder { false };
    bool rollOver { false };
    uint8_t opacity { 50 };

    unsigned backCount { 3 };
    unsigned frontCount { 2 };
};

#endif // ONION_SKIN_STATE_HPP
