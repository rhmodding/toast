#ifndef EDITOR_DATA_PACKAGE_HPP
#define EDITOR_DATA_PACKAGE_HPP

#include "manager/SessionManager.hpp"

// TODO: rewrite this whole thing it's VERY embarrasing

#define TED_ARC_FILENAME "TOAST.TED"
#define TED_ARC_FILENAME_OLD "TOAST.DAT"

void TedApply(const unsigned char* data, const Session& session);

struct TedWriteState;

[[nodiscard]] TedWriteState* TedCreateWriteState(const Session& session);
void TedDestroyWriteState(TedWriteState* state);

// Prepares write state; returns output buffer size
unsigned TedPrepareWrite(TedWriteState* state);

// Write to output buffer. Sizeof buffer must be retval of TedPrepareWrite
void TedWrite(TedWriteState* state, unsigned char* buffer);

#endif // EDITOR_DATA_PACKAGE_HPP
