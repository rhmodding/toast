#ifndef SARC_HPP
#define SARC_HPP

#include "Archive.hpp"

namespace Archive {

class SARCObject : public Archive::ArchiveObjectBase {
public:
    SARCObject(const unsigned char* data, const size_t dataSize);
    SARCObject() = default;

    [[nodiscard]] std::vector<unsigned char> Serialize();
};

} // namespace Archive

#endif // SARC_HPP
