#include "EditorDataPackage.hpp"

#include <cstdint>
#include <cstring>

#include <string>

#include <iostream>

#include <unordered_map>

#include <zlib-ng.h>

#include "Logging.hpp"

#include "cellanim/CellAnim.hpp"

constexpr uint32_t TED_MAGIC = 6132025; // Jun 13 2025

// Major version must be changed for any breaking changes.
constexpr unsigned int TED_VERSION_MAJOR = 1;
constexpr unsigned int TED_VERSION_MINOR = 0;

enum TedFlag {
    TED_FLAG_NONE = 0,
    TED_FLAG_COMPRESSED_ZLIB = (1 << 0),
};

struct TedFileHeader {
    uint32_t magic { TED_MAGIC };
    uint8_t majorVersion { TED_VERSION_MAJOR };
    uint8_t minorVersion { TED_VERSION_MINOR };

    uint16_t headerSize { sizeof(TedFileHeader) };

    uint32_t dataSize { 0 }; // Size of (compressed) data.
    uint32_t decompressedSize { 0 }; // Will be equal to (fileSize - headerSize) if not compressed.

    uint32_t flags { TED_FLAG_NONE }; // See TedFlag.

    // (Compressed) data starts at ((unsigned char*)this + headerSize).
};

enum TedEntryType {
    TED_ENTRY_TYPE_SETFOLLOW_CELLANIM_IDX = 0, // Set the following entries' CellAnim index.
    TED_ENTRY_TYPE_PART_NAME = 1, // Set a part's name.
    TED_ENTRY_TYPE_PART_LOCK = 2, // Set a part to locked.
    TED_ENTRY_TYPE_PART_HIDE = 3, // Set a part to hidden.
};

struct TedEntry {
    uint8_t type; // See TedEntryType.
    uint8_t sizeHalf; // Size of this entry divided by two.
    union {
        struct {
            uint16_t cellAnimIndex;
        } setFollowCellAnimIdx;
        struct {
            uint16_t arrangementIndex;
            uint16_t partIndex;
            uint32_t stringOffset; // Relative to the start of the string table.
        } partName;
        struct {
            uint16_t arrangementIndex;
            uint16_t partIndex;
        } partLock;
        struct {
            uint16_t arrangementIndex;
            uint16_t partIndex;
            // When a part gets set to hidden, it's opacity is overwritten to zero when
            // serialized. The opacity is restored to this value when loaded
            uint8_t originalOpacity;
        } partHide;
    };
};

#define TED_ENTRY_SIZE(typefield) \
    (offsetof(TedEntry, typefield) + sizeof(((TedEntry*)nullptr)->typefield))

struct TedWorkHeader {
    uint32_t entryCount { 0 };
    uint32_t stringPoolOffset { 0 };
    uint32_t reserved[4] { 0, 0, 0, 0 };

    TedEntry firstEntry[0];
};

struct TedFileHeaderOld {
    char magic[5] { 'T', 'O', 'A', 'S', 'T' };

    // Version
    union {
        struct {
            uint8_t majorVersion : 4;
            uint8_t minorVersion : 4;
        };
        uint8_t combinedVersion;
    };

    uint16_t headerSize { sizeof(TedFileHeaderOld) };

    uint16_t entryCount { 0 };
    uint32_t stringPoolOffset { 0 };
} __attribute__((packed));

struct TedSectionHeaderOld {
    uint32_t identifier;
    uint16_t entryCount;
    uint32_t nextSectionOffset;
} __attribute__((packed));

struct TedPartLocationEntryOld {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;
} __attribute__((packed));

struct TedPartOpacityEntryOld {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;

    uint8_t opacity;

    uint8_t _pad8 { 0x00 };
} __attribute__((packed));

struct TedNamedPartEntryOld {
    uint16_t cellanimIndex;
    uint16_t arrangementIndex;
    uint16_t partIndex;

    uint32_t nameOffset; // relative to fileHeader->stringPoolOffset.
} __attribute__((packed));

static CellAnim::ArrangementPart* getPart(
    const Session& session,
    unsigned cellIndex, unsigned arrngIndex, unsigned partIndex
) {
    if (cellIndex >= session.cellanims.size()) {
            Logging::err <<
                "[TedApply] Invalid editor data binary: oob cellanim index!:\n"
                "   - Cellanim Index: " << cellIndex << std::endl;
            return nullptr;
        }

        auto& arrangements = session.cellanims.at(cellIndex).object->getArrangements();
        if (arrngIndex >= arrangements.size()) {
            Logging::err <<
                "[TedApply] Invalid editor data binary: oob arrangement index!:"
                "   - Cellanim Index: " << cellIndex << "\n"
                "   - Arrangement Index: " << arrngIndex << std::endl;
            return nullptr;
        }

        auto& arrangement = arrangements.at(arrngIndex);
        if (partIndex >= arrangement.parts.size()) {
            Logging::err <<
                "[TedApply] Invalid editor data binary: oob part index!:"
                "   - Cellanim Index: " << cellIndex << "\n"
                "   - Arrangement Index: " << arrngIndex << "\n"
                "   - Part Index: " << partIndex << std::endl;
            return nullptr;
        }

        return &arrangement.parts.at(partIndex);
}

static bool ApplyImpl(Session& session, const unsigned char *data, const size_t dataSize) {
    const TedFileHeader* fileHeader = reinterpret_cast<const TedFileHeader*>(data);

    const unsigned char* compressedStart = data + fileHeader->headerSize;
    const unsigned char* compressedEnd = compressedStart + fileHeader->dataSize;

    std::vector<unsigned char> entryData (fileHeader->decompressedSize);
    if (fileHeader->flags & TED_FLAG_COMPRESSED_ZLIB) {
        zng_stream strm {};
        strm.next_in = compressedStart;
        strm.avail_in = compressedEnd - compressedStart;
        strm.next_out = entryData.data();
        strm.avail_out = entryData.size();
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;

        const int init = zng_inflateInit2(&strm, 15);
        if (init != Z_OK) {
            Logging::err << "[EditorDataProc::Apply] zng_inflateInit failed!" << std::endl;
            return false;
        }

        const int ret = zng_inflate(&strm, Z_FINISH);
        zng_inflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            Logging::err << "[EditorDataProc::Apply] zng_inflate failed: " << ret << std::endl;
            return false;
        }
    }
    else {
        entryData.assign(compressedStart, compressedEnd);
    }

    const TedWorkHeader* workHeader = reinterpret_cast<const TedWorkHeader*>(entryData.data());
    const char* stringPool = reinterpret_cast<const char*>(entryData.data() + workHeader->stringPoolOffset);

    const TedEntry* currentEntry = workHeader->firstEntry;
    uint16_t currentCellAnimIdx = 0;

    for (uint32_t i = 0; i < workHeader->entryCount; i++) {
        switch (currentEntry->type) {
        case TED_ENTRY_TYPE_SETFOLLOW_CELLANIM_IDX: {
            currentCellAnimIdx = currentEntry->setFollowCellAnimIdx.cellAnimIndex;
        } break;

        case TED_ENTRY_TYPE_PART_NAME: {
            auto* part = getPart(session,
                currentCellAnimIdx,
                currentEntry->partName.arrangementIndex,
                currentEntry->partName.partIndex
            );
            if (!part)
                return false;

            part->editorName.assign(stringPool + currentEntry->partName.stringOffset);
        } break;

        case TED_ENTRY_TYPE_PART_LOCK: {
            auto* part = getPart(session,
                currentCellAnimIdx,
                currentEntry->partLock.arrangementIndex,
                currentEntry->partLock.partIndex
            );
            if (!part)
                return false;

            part->editorLocked = true;
        } break;

        case TED_ENTRY_TYPE_PART_HIDE: {
            auto* part = getPart(session,
                currentCellAnimIdx,
                currentEntry->partHide.arrangementIndex,
                currentEntry->partHide.partIndex
            );
            if (!part)
                return false;

            part->opacity = currentEntry->partHide.originalOpacity;
            part->editorVisible = false;
        } break;
        
        default:
            break;
        }

        // Advance.
        currentEntry = reinterpret_cast<const TedEntry*>(
            (unsigned char*)currentEntry + (currentEntry->sizeHalf * 2)
        );
    }

    return true;
}

static bool ApplyImpl_Old(Session& session, const unsigned char *data, const size_t dataSize) {
    Logging::warn << "[EditorDataProc::Apply] ---   <<-- USING MALFORM TED HACK -->>   ---" << std::endl;
    Logging::warn << "[EditorDataProc::Apply] TED size is " << (dataSize / 1000) << "kb long" << std::endl;

    const TedFileHeaderOld* fileHeader = reinterpret_cast<const TedFileHeaderOld*>(data);

    uint32_t entryCount = fileHeader->entryCount;

    const TedSectionHeaderOld* lastHeader;
    const TedSectionHeaderOld* currentHeader =
        reinterpret_cast<const TedSectionHeaderOld*>(data + fileHeader->headerSize);

    for (uint32_t i = 0; i < entryCount; i++) {
        if ((unsigned char*)currentHeader >= (data + dataSize)) {
            Logging::warn << "[EditorDataProc::Apply] Stopped processing early: current entry exceeds end of data" << std::endl;
            break;
        }

        uint32_t currentHeaderOffs = ((unsigned char*)currentHeader) - data;

        uint32_t nextHeaderOffs = currentHeader->nextSectionOffset;

        switch (currentHeader->identifier) {
        case IDENTIFIER_TO_U32('E', 'L', 'P', 'T'): {
            const TedPartLocationEntryOld* currentEntry = reinterpret_cast<const TedPartLocationEntryOld*>(
                currentHeader + 1
            );

            uint32_t entryCount = (
                currentHeader->nextSectionOffset - currentHeaderOffs - sizeof(TedSectionHeaderOld)
            ) / sizeof(TedPartLocationEntryOld);

            if (fileHeader->minorVersion == 5) {
                entryCount = currentHeader->entryCount;
                nextHeaderOffs = currentHeaderOffs + sizeof(TedSectionHeaderOld) + (entryCount * sizeof(TedNamedPartEntryOld));
            }

            for (uint32_t j = 0; j < currentHeader->entryCount; j++) {
                auto* part = getPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part)
                    part->editorLocked = true;
                else
                    return false;

                currentEntry++;
            }
        } break;

        case IDENTIFIER_TO_U32('E', 'I', 'P', 'T'): {
            const TedPartOpacityEntryOld* currentEntry = reinterpret_cast<const TedPartOpacityEntryOld*>(
                currentHeader + 1
            );

            uint32_t entryCount = (
                currentHeader->nextSectionOffset - currentHeaderOffs - sizeof(TedSectionHeaderOld)
            ) / sizeof(TedPartOpacityEntryOld);

            if (fileHeader->minorVersion == 5) {
                entryCount = currentHeader->entryCount;
                nextHeaderOffs = currentHeaderOffs + sizeof(TedSectionHeaderOld) + (entryCount * sizeof(TedNamedPartEntryOld));
            }

            for (uint32_t j = 0; j < entryCount; j++) {
                auto* part = getPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part) {
                    part->opacity = currentEntry->opacity;
                    part->editorVisible = false;
                }
                else {
                    return false;
                }

                currentEntry++;
            }
        } break;

        case IDENTIFIER_TO_U32('A', 'P', 'N', 'T'): {
            const TedNamedPartEntryOld* currentEntry = reinterpret_cast<const TedNamedPartEntryOld*>(
                currentHeader + 1
            );

            uint32_t entryCount = (
                currentHeader->nextSectionOffset - currentHeaderOffs - sizeof(TedSectionHeaderOld)
            ) / sizeof(TedNamedPartEntryOld);

            if (fileHeader->minorVersion == 5) {
                entryCount = currentHeader->entryCount;
                nextHeaderOffs = currentHeaderOffs + sizeof(TedSectionHeaderOld) + (entryCount * sizeof(TedNamedPartEntryOld));
            }

            for (uint32_t j = 0; j < entryCount; j++) {
                auto* part = getPart(
                    session,
                    currentEntry->cellanimIndex,
                    currentEntry->arrangementIndex,
                    currentEntry->partIndex
                );

                if (part) {
                    part->editorName.assign(
                        (char*)data + fileHeader->stringPoolOffset + currentEntry->nameOffset
                    );
                }
                else
                    return false;

                currentEntry++;
            }
        } break;

        default: {
            Logging::err << "[EditorDataProc::Apply] Unimplemented entry type: " <<
                ((char*)&currentHeader->identifier)[0] <<
                ((char*)&currentHeader->identifier)[1] <<
                ((char*)&currentHeader->identifier)[2] <<
                ((char*)&currentHeader->identifier)[3] << std::endl;
        } break;
        }

        if (i + 1 < entryCount) {
            lastHeader = currentHeader;
            currentHeader =
                reinterpret_cast<const TedSectionHeaderOld*>(data + nextHeaderOffs);
        }
    }

    return true;
}

bool EditorDataProc::Apply(Session& session, const unsigned char *data, const size_t dataSize) {
    const TedFileHeader* fileHeader = reinterpret_cast<const TedFileHeader*>(data);

    bool isOldFormat = false;
    if (fileHeader->magic != TED_MAGIC) {
        const TedFileHeaderOld* oldFileHeader = reinterpret_cast<const TedFileHeaderOld*>(data);
        if (memcmp(oldFileHeader->magic, "TOAST", 5) == 0) {
            isOldFormat = true;
        }
        else {
            Logging::err << "[EditorDataProc::Apply] Invalid editor data binary: header magic failed check!" << std::endl;
            return false;
        }
    }

    if (isOldFormat) {
        return ApplyImpl_Old(session, data, dataSize);
    }

    if (fileHeader->majorVersion != TED_VERSION_MAJOR) {
        Logging::err <<
            "[EditorDataProc::Apply] Invalid editor data binary: version not supported (expected "
            << TED_VERSION_MAJOR << ".xx, got " << (unsigned)fileHeader->majorVersion << '.' <<
            (unsigned)fileHeader->minorVersion << ')' << std::endl;
        return false;
    }

    return ApplyImpl(session, data, dataSize);
}

std::vector<unsigned char> EditorDataProc::Create(const Session &session) {
    // string - offset into pool
    std::unordered_map<std::string, unsigned> stringPoolMap;
    uint32_t nextStringPoolOffs = 0;

    std::vector<TedEntry> entries;

    auto trySetFollowCellAnim = [&](bool& alreadySet, size_t idx) {
        if (!alreadySet) {
            entries.push_back(TedEntry{
                .type = TED_ENTRY_TYPE_SETFOLLOW_CELLANIM_IDX,
                .sizeHalf = TED_ENTRY_SIZE(setFollowCellAnimIdx) / 2,
                .setFollowCellAnimIdx = {
                    .cellAnimIndex = static_cast<uint16_t>(idx)
                }
            });
            alreadySet = true;
        }
    };

    for (size_t cellAnimIdx = 0; cellAnimIdx < session.cellanims.size(); cellAnimIdx++) {
        bool alreadySetFollowCellAnim = (cellAnimIdx == 0);

        const auto& arrangements = session.cellanims[cellAnimIdx].object->getArrangements();
        for (size_t arrangementIdx = 0; arrangementIdx < arrangements.size(); arrangementIdx++) {
            const auto& parts = arrangements[arrangementIdx].parts;
            for (size_t partIdx = 0; partIdx < parts.size(); partIdx++) {
                const auto& part = parts[partIdx];

                if (!part.editorName.empty()) {
                    trySetFollowCellAnim(alreadySetFollowCellAnim, cellAnimIdx);

                    TedEntry entry;
                    entry.type = TED_ENTRY_TYPE_PART_NAME;
                    entry.sizeHalf = TED_ENTRY_SIZE(partName) / 2;
                    entry.partName.arrangementIndex = static_cast<uint16_t>(arrangementIdx);
                    entry.partName.partIndex = static_cast<uint16_t>(partIdx);

                    auto [it, inserted] = stringPoolMap.emplace(part.editorName, nextStringPoolOffs);
                    if (inserted) {
                        entry.partName.stringOffset = nextStringPoolOffs;
                        nextStringPoolOffs += part.editorName.size() + 1;
                    } else {
                        entry.partName.stringOffset = it->second;
                    }

                    entries.push_back(entry);
                }

                if (part.editorLocked) {
                    trySetFollowCellAnim(alreadySetFollowCellAnim, cellAnimIdx);

                    TedEntry entry;
                    entry.type = TED_ENTRY_TYPE_PART_LOCK;
                    entry.sizeHalf = TED_ENTRY_SIZE(partLock) / 2;
                    entry.partLock.arrangementIndex = static_cast<uint16_t>(arrangementIdx);
                    entry.partLock.partIndex = static_cast<uint16_t>(partIdx);

                    entries.push_back(entry);
                }

                if (!part.editorVisible) {
                    trySetFollowCellAnim(alreadySetFollowCellAnim, cellAnimIdx);

                    TedEntry entry;
                    entry.type = TED_ENTRY_TYPE_PART_HIDE;
                    entry.sizeHalf = TED_ENTRY_SIZE(partHide) / 2;
                    entry.partHide.arrangementIndex = static_cast<uint16_t>(arrangementIdx);
                    entry.partHide.partIndex = static_cast<uint16_t>(partIdx);
                    entry.partHide.originalOpacity = part.opacity;

                    entries.push_back(entry);
                }
            }
        }
    }

    size_t workDataSize = sizeof(TedWorkHeader);
    for (size_t i = 0; i < entries.size(); i++) {
        workDataSize += entries[i].sizeHalf * 2;
    }

    size_t stringPoolOffset = workDataSize;
    workDataSize += nextStringPoolOffs;

    std::vector<unsigned char> workData (workDataSize);

    TedWorkHeader* workHeader = reinterpret_cast<TedWorkHeader*>(workData.data());

    workHeader->entryCount = static_cast<uint32_t>(entries.size());
    workHeader->stringPoolOffset = static_cast<uint32_t>(stringPoolOffset);
    std::memset(workHeader->reserved, 0x00, sizeof(workHeader->reserved));

    unsigned char* currentEntryPos = reinterpret_cast<unsigned char*>(workHeader->firstEntry);
    for (size_t i = 0; i < entries.size(); i++) {
        const auto& entry = entries[i];
        unsigned entrySize = entry.sizeHalf * 2;

        std::memcpy(currentEntryPos, &entry, entrySize);
        currentEntryPos += entrySize;        
    }

    char* stringPoolStart = reinterpret_cast<char*>(workData.data() + stringPoolOffset);
    for (const auto& [name, offset] : stringPoolMap) {
        std::memcpy(stringPoolStart + offset, name.c_str(), name.size() + 1);
    }

    stringPoolMap.clear(); // Done using it.

    std::vector<unsigned char> compressedWork (zng_compressBound(workDataSize));

    zng_stream strm {};
    strm.next_in = workData.data();
    strm.avail_in = workData.size();
    strm.next_out = compressedWork.data();
    strm.avail_out = compressedWork.size();
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    const int init = zng_deflateInit2(&strm, Z_BEST_COMPRESSION, 8, 15, 8, 0);
    if (init != Z_OK) {
        Logging::err << "[EditorDataProc::Create] zng_deflateInit failed!" << std::endl;
        return std::vector<unsigned char>();
    }

    const int ret = zng_deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        Logging::err << "[EditorDataProc::Create] zng_deflate failed: " << ret << std::endl;

        zng_deflateEnd(&strm);
        return std::vector<unsigned char>();
    }

    zng_deflateEnd(&strm);

    compressedWork.resize(strm.total_out);

    workData.clear(); // Done using it.

    std::vector<unsigned char> resultData(sizeof(TedFileHeader) + compressedWork.size());

    TedFileHeader* fileHeader = reinterpret_cast<TedFileHeader*>(resultData.data());
    *fileHeader = TedFileHeader {};

    fileHeader->dataSize = static_cast<uint32_t>(compressedWork.size());
    fileHeader->decompressedSize = static_cast<uint32_t>(workDataSize);

    fileHeader->flags = TED_FLAG_COMPRESSED_ZLIB;

    unsigned char* compressedDataStart = reinterpret_cast<unsigned char*>(fileHeader + 1);

    std::memcpy(compressedDataStart, compressedWork.data(), compressedWork.size());

    return resultData;
}
