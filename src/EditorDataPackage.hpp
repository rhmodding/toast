#ifndef EDITOR_DATA_PACKAGE_HPP
#define EDITOR_DATA_PACKAGE_HPP

#include "SessionManager.hpp"

#define TED_ARC_FILENAME "TOAST.TED"
#define TED_ARC_FILENAME_OLD "TOAST.DAT"

void TedApply(const unsigned char* data, SessionManager::Session* session);

unsigned TedPrecomputeSize(SessionManager::Session* session);
void TedWrite(SessionManager::Session* session, unsigned char* output);

#endif // EDITOR_DATA_PACKAGE_HPP
