#ifndef EDITOR_DATA_PACKAGE_HPP
#define EDITOR_DATA_PACKAGE_HPP

#include "manager/SessionManager.hpp"

#include <cstddef>

#include <string_view>

namespace EditorDataProc {

inline constexpr std::string_view ARCHIVE_FILENAME = "TOAST.TED";

bool Apply(Session& session, const unsigned char* data, size_t dataSize);

std::vector<unsigned char> Create(const Session& session);

} // namespace EditorDataProc

#endif // EDITOR_DATA_PACKAGE_HPP
