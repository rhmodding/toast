#ifndef EDITOR_DATA_PACKAGE_HPP
#define EDITOR_DATA_PACKAGE_HPP

#include "SessionManager.hpp"

#define TED_ARC_FILENAME "TOAST.TED"
#define TED_ARC_FILENAME_OLD "TOAST.DAT"

void TedApply(const unsigned char* data, const SessionManager::Session& session);

struct TedWriteState;

unsigned TedPrepareWrite(TedWriteState** state, const SessionManager::Session& session);
void TedWrite(TedWriteState* state, unsigned char* buffer);

#endif // EDITOR_DATA_PACKAGE_HPP
