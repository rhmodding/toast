#ifndef CRC32_HPP
#define CRC32_HPP

#include <cstdint>

#include <array>

#include <string_view>

namespace CRC32 {

constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320;

constexpr std::array<uint32_t, 256> generateTable() {
    std::array<uint32_t, 256> table {};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (unsigned j = 0; j < 8; ++j)
            crc = (crc & 1) ? (crc >> 1) ^ CRC32_POLYNOMIAL : (crc >> 1);

        table[i] = crc;
    }

    return table;
}

constexpr auto CRC32_TABLE = generateTable();

constexpr uint32_t compute(std::string_view data, uint32_t crc = 0xFFFFFFFF) {
    for (unsigned char byte : data)
        crc = (crc >> 8) ^ CRC32_TABLE[(crc ^ byte) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

} // namespace CRC32

#endif // CRC32_HPP
