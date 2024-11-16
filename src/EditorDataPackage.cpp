#include "EditorDataPackage.hpp"

#include <cstdint>
#include <cstring>

#include <unordered_set>
#include <string>

#include <iostream>

#include "anim/RvlCellAnim.hpp"

// Major version must be changed if breaking.
#define TED_VERSION_MAJOR (0) // 0 - 16
// Minor version must be changed if for example extra sections are introduced.
#define TED_VERSION_MINOR (0) // 0 - 16

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
    char identifier[4];
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
    SessionManager::Session* session,
    uint16_t cellIndex, uint16_t arrngIndex, uint16_t partIndex
) {
    if (cellIndex >= session->cellanims.size()) {
            std::cerr <<
                "[TedApply] Invalid editor data binary: oob cellanim index!:\n" <<
                "   - Cellanim Index: " << cellIndex << '\n';
            return nullptr;
        }

        auto& arrangements = session->cellanims.at(cellIndex).object->arrangements;
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

static void _TedApply(const unsigned char* data, SessionManager::Session* session) {
    const TedFileHeader* fileHeader = reinterpret_cast<const TedFileHeader*>(data);
    if (fileHeader->entryCount == 0)
        return;

    const TedSectionHeader* currentHeader =
        reinterpret_cast<const TedSectionHeader*>(data + fileHeader->headerSize);

    for (uint16_t i = 0; i < fileHeader->entryCount; i++) {
        switch (*reinterpret_cast<const uint32_t*>(currentHeader->identifier)) {
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
                            32
                        );
                    else
                        return;

                    currentEntry++;
                }
            } break;

            default:
                std::cerr << "[TedApply] Unimplemented entry type: " <<
                    currentHeader->identifier[0] <<
                    currentHeader->identifier[1] <<
                    currentHeader->identifier[2] <<
                    currentHeader->identifier[3] <<
                '\n';
                break;
        }

        if (i + 1 < fileHeader->entryCount)
            currentHeader =
                reinterpret_cast<const TedSectionHeader*>(data + currentHeader->nextSectionOffset);
    }
}

static void _TedApplyOld(const unsigned char* data, SessionManager::Session* session) {
    const TedFileHeaderOld* fileHeader = reinterpret_cast<const TedFileHeaderOld*>(data);
    if (fileHeader->entryCount == 0)
        return;

    const TedSectionHeader* currentHeader =
        reinterpret_cast<const TedSectionHeader*>(fileHeader + 1);

    for (uint16_t i = 0; i < fileHeader->entryCount; i++) {
        switch (*reinterpret_cast<const uint32_t*>(currentHeader->identifier)) {
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
                        strncpy(part->editorName, currentEntry->name, 32);
                    else
                        return;

                    currentEntry++;
                }
            } break;

            default:
                std::cerr << "[TedApply] Unimplemented entry type: " <<
                    currentHeader->identifier[0] <<
                    currentHeader->identifier[1] <<
                    currentHeader->identifier[2] <<
                    currentHeader->identifier[3] <<
                '\n';
                break;
        }

        if (i + 1 < fileHeader->entryCount)
            currentHeader =
                reinterpret_cast<const TedSectionHeader*>(data + currentHeader->nextSectionOffset);
    }
}

void TedApply(const unsigned char* data, SessionManager::Session* session) {
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

unsigned TedPrecomputeSize(SessionManager::Session* session) {
    unsigned size = sizeof(TedFileHeader);
    unsigned stringsSize = 0;

    bool hasLocationSection { false };
    bool hasOpacitySection { false };
    bool hasNameSection { false };

    std::unordered_set<uint32_t> nameHashes;

    for (uint16_t i = 0; i < session->cellanims.size(); i++) {
        const auto cellanim = session->cellanims[i].object;

        for (uint16_t j = 0; j < cellanim->arrangements.size(); j++) {
            const auto& arrangement = cellanim->arrangements[j];

            for (uint16_t n = 0; n < arrangement.parts.size(); n++) {
                const auto& part = arrangement.parts[n];

                if (part.editorLocked) {
                    if (!hasLocationSection) {
                        size += sizeof(TedSectionHeader);
                        hasLocationSection = true;
                    }
                    size += sizeof(TedPartLocationEntry);
                }
                if (!part.editorVisible) {
                    if (!hasOpacitySection) {
                        size += sizeof(TedSectionHeader);
                        hasOpacitySection = true;
                    }
                    size += sizeof(TedPartOpacityEntry);
                }

                if (part.editorName[0] != '\0') {
                    if (!hasNameSection) {
                        size += sizeof(TedSectionHeader);
                        hasNameSection = true;
                    }

                    size += sizeof(TedNamedPartEntry);

                    // Check if name is unique
                    uint32_t nameHash = 0;
                    for (unsigned i = 0; i < strlen(part.editorName); i++)
                        nameHash = part.editorName[i] + nameHash * 0x65;

                    if (nameHashes.find(nameHash) == nameHashes.end()) {
                        nameHashes.insert(nameHash);

                        stringsSize += strlen(part.editorName) + 1;
                        stringsSize = (stringsSize + 3) & ~3;
                    }
                }
            }
        }
    }

    return size + stringsSize;
}
void TedWrite(SessionManager::Session* session, unsigned char* output) {
    TedFileHeader* fileHeader = reinterpret_cast<TedFileHeader*>(output);
    *fileHeader = TedFileHeader {};

    fileHeader->majorVersion = TED_VERSION_MAJOR;
    fileHeader->minorVersion = TED_VERSION_MINOR;

    std::vector<TedPartLocationEntry> lockedParts;
    std::vector<TedPartOpacityEntry> invisibleParts;
    std::vector<TedNamedPartEntry> namedParts;

    unsigned nextNameOffset { 0 };
    std::unordered_map<std::string, unsigned> nameOffsets;

    for (uint16_t i = 0; i < session->cellanims.size(); i++) {
        const auto cellanim = session->cellanims[i].object;

        for (uint16_t j = 0; j < cellanim->arrangements.size(); j++) {
            const auto& arrangement = cellanim->arrangements[j];

            for (uint16_t n = 0; n < arrangement.parts.size(); n++) {
                const auto& part = arrangement.parts[n];

                if (part.editorLocked)
                    lockedParts.push_back({ i, j, n });
                if (!part.editorVisible)
                    invisibleParts.push_back({ i, j, n, part.opacity });

                if (part.editorName[0] != '\0') {
                    TedNamedPartEntry entry {
                        .cellanimIndex = i,
                        .arrangementIndex = j,
                        .partIndex = n
                    };

                    std::string name(part.editorName);

                    // Name is unique, add to map
                    if (nameOffsets.count(name) == 0) {
                        entry.nameOffset = nextNameOffset;
                        nameOffsets[name] = nextNameOffset;

                        nextNameOffset += strlen(part.editorName) + 1;
                        nextNameOffset = (nextNameOffset + 3) & ~3;
                    }
                    // Name not unique, reuse offset
                    else {
                        entry.nameOffset = nameOffsets[name];
                    }

                    namedParts.push_back(entry);
                }
            }
        }
    }

    unsigned writeOffset { sizeof(TedFileHeader) };

    if (!invisibleParts.empty()) {
        fileHeader->entryCount++;

        uint32_t endOffset =
            writeOffset + sizeof(TedSectionHeader) +
            (sizeof(TedPartOpacityEntry) * invisibleParts.size());

        TedSectionHeader* header =
            reinterpret_cast<TedSectionHeader*>(output + writeOffset);
        *header = TedSectionHeader {
            .identifier = { 'E', 'I', 'P', 'T' },
            .entryCount = (uint16_t)invisibleParts.size(),
            .nextSectionOffset = endOffset
        };

        writeOffset += sizeof(TedSectionHeader);

        for (unsigned i = 0; i < header->entryCount; i++) {
            *reinterpret_cast<TedPartOpacityEntry*>(
                output + writeOffset
            ) = invisibleParts[i];

            writeOffset += sizeof(TedPartOpacityEntry);
        }
    }

    if (!lockedParts.empty()) {
        fileHeader->entryCount++;

        uint32_t endOffset =
            writeOffset + sizeof(TedSectionHeader) +
            (sizeof(TedPartLocationEntry) * lockedParts.size());

        TedSectionHeader* header =
            reinterpret_cast<TedSectionHeader*>(output + writeOffset);
        *header = TedSectionHeader {
            .identifier = { 'E', 'L', 'P', 'T' },
            .entryCount = (uint16_t)lockedParts.size(),
            .nextSectionOffset = endOffset
        };

        writeOffset += sizeof(TedSectionHeader);

        for (unsigned i = 0; i < header->entryCount; i++) {
            *reinterpret_cast<TedPartLocationEntry*>(
                output + writeOffset
            ) = lockedParts[i];

            writeOffset += sizeof(TedPartLocationEntry);
        }
    }

    if (!namedParts.empty()) {
        fileHeader->entryCount++;

        uint32_t endOffset =
            writeOffset + sizeof(TedSectionHeader) +
            (sizeof(TedNamedPartEntry) * namedParts.size());

        TedSectionHeader* header =
            reinterpret_cast<TedSectionHeader*>(output + writeOffset);
        *header = TedSectionHeader {
            .identifier = { 'A', 'P', 'N', 'T' },
            .entryCount = (uint16_t)namedParts.size(),
            .nextSectionOffset = endOffset
        };

        writeOffset += sizeof(TedSectionHeader);

        for (unsigned i = 0; i < header->entryCount; i++) {
            *reinterpret_cast<TedNamedPartEntry*>(
                output + writeOffset
            ) = namedParts[i];

            writeOffset += sizeof(TedNamedPartEntry);
        }
    }

    fileHeader->stringPoolOffset = writeOffset;

    // Write names
    for (const auto& [name, offset] : nameOffsets) {
        strcpy((char*)output + writeOffset + offset, name.c_str());
    }
}
