#include "EditorDataPackage.hpp"

#include <cstdint>
#include <cstring>

#include <unordered_set>
#include <string>

#include <iostream>

#include "anim/RvlCellAnim.hpp"

// Major version must be changed for any breaking changes.
constexpr unsigned int TED_VERSION_MAJOR = 0;
constexpr unsigned int TED_VERSION_MINOR = 0;

static_assert(TED_VERSION_MAJOR < 16, "TED_VERSION_MAJOR must be less than 16");
static_assert(TED_VERSION_MINOR < 16, "TED_VERSION_MINOR must be less than 16");

struct TedFileHeader {
    char magic[5] { 'T', 'O', 'A', 'S', 'T' };
    
    // Version
    union {
        struct {
            uint8_t majorVersion : 4;
            uint8_t minorVersion : 4;
        };
        uint8_t combinedVersion;
    };

    uint16_t headerSize { sizeof(TedFileHeader) };

    uint16_t entryCount { 0 };
    uint32_t stringPoolOffset { 0 };
} __attribute__((packed));

struct TedFileHeaderOld {
    char magic[6] { 'T', 'O', 'A', 'S', 'T', 'D' };
    uint16_t entryCount { 0 };
} __attribute__((packed));

struct TedSectionHeader {
    uint32_t identifier;
    uint16_t entryCount;
    uint32_t nextSectionOffset;
} __attribute__((packed));

struct TedPartLocationEntry {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;
} __attribute__((packed));

struct TedPartOpacityEntry {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;

    uint8_t opacity;

    uint8_t _pad8 { 0x00 };
} __attribute__((packed));

struct TedNamedPartEntry {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;

    uint32_t nameOffset; // relative to fileHeader->stringPoolOffset.
} __attribute__((packed));

struct TedNamedPartEntryOld {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;

    char name[32];
} __attribute__((packed));

static RvlCellAnim::ArrangementPart* _TedGetPart(
    const Session& session,
    uint16_t cellIndex, uint16_t arrngIndex, uint16_t partIndex
) {
    if (cellIndex >= session.cellanims.size()) {
            std::cerr <<
                "[TedApply] Invalid editor data binary: oob cellanim index!:\n" <<
                "   - Cellanim Index: " << cellIndex << '\n';
            return nullptr;
        }

        auto& arrangements = session.cellanims.at(cellIndex).object->arrangements;
        if (arrngIndex >= arrangements.size()) {
            std::cerr <<
                "[TedApply] Invalid editor data binary: oob arrangement index!:" <<
                "   - Cellanim Index: " << cellIndex << '\n' <<
                "   - Arrangement Index: " << arrngIndex << '\n';
            return nullptr;
        }

        auto& arrangement = arrangements.at(arrngIndex);
        if (partIndex >= arrangement.parts.size()) {
            std::cerr <<
                "[TedApply] Invalid editor data binary: oob part index!:" <<
                "   - Cellanim Index: " << cellIndex << '\n' <<
                "   - Arrangement Index: " << arrngIndex << '\n' <<
                "   - Part Index: " << partIndex << '\n';
            return nullptr;
        }

        return &arrangement.parts.at(partIndex);
}

static void _TedApply(const unsigned char* data, const Session& session) {
    const TedFileHeader* fileHeader = reinterpret_cast<const TedFileHeader*>(data);
    if (fileHeader->entryCount == 0)
        return;

    const TedSectionHeader* currentHeader =
        reinterpret_cast<const TedSectionHeader*>(data + fileHeader->headerSize);

    for (uint16_t i = 0; i < fileHeader->entryCount; i++) {
        switch (currentHeader->identifier) {
        case IDENTIFIER_TO_U32('E', 'L', 'P', 'T'): {
            const TedPartLocationEntry* currentEntry = reinterpret_cast<const TedPartLocationEntry*>(
                currentHeader + 1
            );
            for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = _TedGetPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part)
                    part->editorLocked = true;
                else
                    return;

                currentEntry++;
            }
        } break;

        case IDENTIFIER_TO_U32('E', 'I', 'P', 'T'): {
            const TedPartOpacityEntry* currentEntry = reinterpret_cast<const TedPartOpacityEntry*>(
                currentHeader + 1
            );
            for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = _TedGetPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part) {
                    part->opacity = currentEntry->opacity;
                    part->editorVisible = false;
                }
                else
                    return;

                currentEntry++;
            }
        } break;

        case IDENTIFIER_TO_U32('A', 'P', 'N', 'T'): {
            const TedNamedPartEntry* currentEntry = reinterpret_cast<const TedNamedPartEntry*>(
                currentHeader + 1
            );
            for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = _TedGetPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part)
                    strncpy(
                        part->editorName,
                        (char*)data + fileHeader->stringPoolOffset + currentEntry->nameOffset,
                        sizeof(part->editorName)
                    );
                else
                    return;

                currentEntry++;
            }
        } break;

        default:
            std::cerr << "[TedApply] Unimplemented entry type: " <<
                ((char*)&currentHeader->identifier)[0] <<
                ((char*)&currentHeader->identifier)[1] <<
                ((char*)&currentHeader->identifier)[2] <<
                ((char*)&currentHeader->identifier)[3] <<
            '\n';
            break;
        }

        if (i + 1 < fileHeader->entryCount)
            currentHeader =
                reinterpret_cast<const TedSectionHeader*>(data + currentHeader->nextSectionOffset);
    }
}

static void _TedApplyOld(const unsigned char* data, const Session& session) {
    const TedFileHeaderOld* fileHeader = reinterpret_cast<const TedFileHeaderOld*>(data);
    if (fileHeader->entryCount == 0)
        return;

    const TedSectionHeader* currentHeader =
        reinterpret_cast<const TedSectionHeader*>(fileHeader + 1);

    for (uint16_t i = 0; i < fileHeader->entryCount; i++) {
        switch (currentHeader->identifier) {
        case IDENTIFIER_TO_U32('E', 'L', 'P', 'T'): {
            const TedPartLocationEntry* currentEntry = reinterpret_cast<const TedPartLocationEntry*>(
                currentHeader + 1
            );
            for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = _TedGetPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part)
                    part->editorLocked = true;
                else
                    return;

                currentEntry++;
            }
        } break;

        case IDENTIFIER_TO_U32('E', 'I', 'P', 'T'): {
            const TedPartOpacityEntry* currentEntry = reinterpret_cast<const TedPartOpacityEntry*>(
                currentHeader + 1
            );
            for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = _TedGetPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part) {
                    part->opacity = currentEntry->opacity;
                    part->editorVisible = false;
                }
                else
                    return;

                currentEntry++;
            }
        } break;

        case IDENTIFIER_TO_U32('A', 'P', 'N', 'T'): {
            const TedNamedPartEntryOld* currentEntry = reinterpret_cast<const TedNamedPartEntryOld*>(
                currentHeader + 1
            );
            for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = _TedGetPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part)
                    strncpy(part->editorName, currentEntry->name, sizeof(part->editorName));
                else
                    return;

                currentEntry++;
            }
        } break;

        default:
            std::cerr << "[TedApply] Unimplemented entry type: " <<
                ((char*)&currentHeader->identifier)[0] <<
                ((char*)&currentHeader->identifier)[1] <<
                ((char*)&currentHeader->identifier)[2] <<
                ((char*)&currentHeader->identifier)[3] <<
            '\n';
            break;
        }

        if (i + 1 < fileHeader->entryCount)
            currentHeader =
                reinterpret_cast<const TedSectionHeader*>(data + currentHeader->nextSectionOffset);
    }
}

void TedApply(const unsigned char* data, const Session& session) {
    const TedFileHeader* fileHeader = reinterpret_cast<const TedFileHeader*>(data);

    if (memcmp(fileHeader->magic, "TOAST", 5) != 0) {
        std::cerr << "[TedApply] Invalid editor data binary: header magic failed check!\n";
        return;
    }

    // Old magic was 6 characters "TOASTD", so we check the last character for D
    // TODO: remove this on release
    if (fileHeader->magic[5] == 'D') {
        std::cout <<
            "[TedApply] An older TED version has been provided. Support for this will be dropped on final release.\n" <<
            "           Please migrate your cellanim projects by opening them and resaving.\n";

        _TedApplyOld(data, session);
        return;
    }

    if (fileHeader->majorVersion != TED_VERSION_MAJOR) {
        std::cerr <<
            "[TedApply] Invalid editor data binary: version not supported (expected "
            << TED_VERSION_MAJOR << ".xx, got " << (unsigned)fileHeader->majorVersion << '.' <<
            (unsigned)fileHeader->minorVersion << ")\n";
        return;
    }

    _TedApply(data, session);
}

struct TedWriteState {
    std::vector<TedPartLocationEntry> lockedParts;
    std::vector<TedPartOpacityEntry> invisibleParts;
    std::vector<TedNamedPartEntry> namedParts;

    std::unordered_map<std::string, unsigned> nameOffsets;

    const Session& session;
};

TedWriteState* TedCreateWriteState(const Session& session) {
    return new TedWriteState {
        .session = session
    };
}
void TedDestroyWriteState(TedWriteState* state) {
    delete state;
}

// Returns size for buffer
unsigned TedPrepareWrite(TedWriteState* state) {
    unsigned size = sizeof(TedFileHeader);
    unsigned nextNameOffset = 0;

    for (uint16_t i = 0; i < state->session.cellanims.size(); i++) {
        const std::shared_ptr cellanim = state->session.cellanims[i].object;

        for (uint16_t j = 0; j < cellanim->arrangements.size(); j++) {
            const auto& arrangement = cellanim->arrangements[j];

            for (uint16_t n = 0; n < arrangement.parts.size(); n++) {
                const auto& part = arrangement.parts[n];

                if (part.editorLocked)
                    state->lockedParts.push_back({ i, j, n });
                if (!part.editorVisible)
                    state->invisibleParts.push_back({ i, j, n, part.opacity });

                if (part.editorName[0] != '\0') {
                    TedNamedPartEntry entry {
                        .cellanimIndex = i,
                        .arrangementIndex = j,
                        .partIndex = n
                    };

                    std::string name(part.editorName);

                    // Name is unique, add to map
                    if (state->nameOffsets.count(name) == 0) {
                        entry.nameOffset = nextNameOffset;
                        state->nameOffsets[name] = nextNameOffset;

                        nextNameOffset += strlen(part.editorName) + 1;
                        nextNameOffset = (nextNameOffset + 3) & ~3;
                    }
                    // Name not unique, reuse offset
                    else {
                        entry.nameOffset = state->nameOffsets[name];
                    }

                    state->namedParts.push_back(entry);
                }
            }
        }
    }

    if (!state->lockedParts.empty()) {
        size +=
            sizeof(TedSectionHeader) +
            (sizeof(TedPartLocationEntry) * state->lockedParts.size());
    }
    if (!state->invisibleParts.empty()) {
        size +=
            sizeof(TedSectionHeader) +
            (sizeof(TedPartOpacityEntry) * state->invisibleParts.size());
    }
    if (!state->namedParts.empty()) {
        size +=
            sizeof(TedSectionHeader) +
            (sizeof(TedNamedPartEntry) * state->namedParts.size());
    }

    return size + nextNameOffset;
}
void TedWrite(TedWriteState* state, unsigned char* buffer) {
    TedFileHeader* fileHeader = reinterpret_cast<TedFileHeader*>(buffer);
    *fileHeader = TedFileHeader {};

    fileHeader->majorVersion = TED_VERSION_MAJOR;
    fileHeader->minorVersion = TED_VERSION_MINOR;

    unsigned char* writeData = buffer + sizeof(TedFileHeader);

    if (!state->invisibleParts.empty()) {
        fileHeader->entryCount++;

        uint32_t endOffset =
            (writeData - buffer) + sizeof(TedSectionHeader) +
            (sizeof(TedPartOpacityEntry) * state->invisibleParts.size());

        TedSectionHeader* header =
            reinterpret_cast<TedSectionHeader*>(writeData);
        *header = TedSectionHeader {
            .identifier = IDENTIFIER_TO_U32('E','I','P','T'),
            .entryCount = static_cast<uint16_t>(state->invisibleParts.size()),
            .nextSectionOffset = endOffset
        };
        writeData += sizeof(TedSectionHeader);

        for (unsigned i = 0; i < header->entryCount; i++) {
            *reinterpret_cast<TedPartOpacityEntry*>(writeData) = state->invisibleParts[i];
            writeData += sizeof(TedPartOpacityEntry);
        }
    }

    if (!state->lockedParts.empty()) {
        fileHeader->entryCount++;

        uint32_t endOffset =
            (writeData - buffer) + sizeof(TedSectionHeader) +
            (sizeof(TedPartLocationEntry) * state->lockedParts.size());

        TedSectionHeader* header =
            reinterpret_cast<TedSectionHeader*>(writeData);
        *header = TedSectionHeader {
            .identifier = IDENTIFIER_TO_U32('E','L','P','T'),
            .entryCount = static_cast<uint16_t>(state->lockedParts.size()),
            .nextSectionOffset = endOffset
        };
        writeData += sizeof(TedSectionHeader);

        for (unsigned i = 0; i < header->entryCount; i++) {
            *reinterpret_cast<TedPartLocationEntry*>(writeData) = state->lockedParts[i];
            writeData += sizeof(TedPartLocationEntry);
        }
    }

    if (!state->namedParts.empty()) {
        fileHeader->entryCount++;

        uint32_t endOffset =
            (writeData - buffer) + sizeof(TedSectionHeader) +
            (sizeof(TedNamedPartEntry) * state->namedParts.size());

        TedSectionHeader* header =
            reinterpret_cast<TedSectionHeader*>(writeData);
        *header = TedSectionHeader {
            .identifier = IDENTIFIER_TO_U32('A','P','N','T'),
            .entryCount = static_cast<uint16_t>(state->namedParts.size()),
            .nextSectionOffset = endOffset
        };
        writeData += sizeof(TedSectionHeader);

        for (unsigned i = 0; i < header->entryCount; i++) {
            *reinterpret_cast<TedNamedPartEntry*>(writeData) = state->namedParts[i];
            writeData += sizeof(TedNamedPartEntry);
        }
    }

    fileHeader->stringPoolOffset = (writeData - buffer);
    for (const auto& [name, offset] : state->nameOffsets)
        strcpy((char*)writeData + offset, name.c_str());
}
