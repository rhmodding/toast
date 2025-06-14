#ifndef EDITOR_DATA_PACKAGE_HPP
#define EDITOR_DATA_PACKAGE_HPP

#include "manager/SessionManager.hpp"

#include <cstddef>

#define TED_ARC_FILENAME "TOAST.TED"
#define TED_ARC_FILENAME_OLD "TOAST.DAT"

namespace EditorDataProc {

bool Apply(Session& session, const unsigned char* data, size_t dataSize);

std::vector<unsigned char> Create(const Session& session);

} // namespace EditorDataProc

#endif // EDITOR_DATA_PACKAGE_HPP
