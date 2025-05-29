#ifndef DARCH_HPP
#define DARCH_HPP

#include "Archive.hpp"

namespace Archive {

class DARCHObject : public Archive::ArchiveObjectBase {
public:
    DARCHObject(const unsigned char* data, const size_t dataSize);
    DARCHObject() = default;

    [[nodiscard]] std::vector<unsigned char> Serialize();
};

} // namespace Archive

#endif // DARCH_HPP
