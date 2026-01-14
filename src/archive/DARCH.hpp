#ifndef DARCH_HPP
#define DARCH_HPP

#include "Archive.hpp"

#include <cstddef>

#include <vector>

namespace Archive {

class DARCHObject : public Archive::ArchiveObjectBase {
public:
    DARCHObject(const unsigned char* data, const size_t dataSize);
    DARCHObject() = default;

    [[nodiscard]] std::vector<unsigned char> serialize();
};

} // namespace Archive

#endif // DARCH_HPP
