#include "SessionManager.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <sstream>
#include <fstream>

#include <filesystem>

#include "anim/RvlCellAnim.hpp"

#include "archive/U8.hpp"
#include "compression/Yaz0.hpp"

#include "texture/TPL.hpp"

#include "ConfigManager.hpp"
#include "PlayerManager.hpp"
#include "MtCommandManager.hpp"

#include "AppState.hpp"

#include "common.hpp"

#define EDITORDATA_RESOURCE_NAME "TOAST.DAT"

namespace EditorData {
    struct FileHeader {
        char magic[6]{ 'T', 'O', 'A', 'S', 'T', 'D' };
        uint16_t entryCount{ 0 };
    } __attribute__((packed));

    struct StandardHeader {
        char identifier[4];
        uint16_t entryCount;
        uint32_t nextHeaderOffset;
    } __attribute__((packed));

    struct PartLocationEntry {
        uint16_t cellanimIndex;
        uint16_t arrangementIndex;
        uint16_t partIndex;
    } __attribute__((packed));

    struct PartAlphaEntry {
        uint16_t cellanimIndex;
        uint16_t arrangementIndex;
        uint16_t partIndex;

        uint8_t opacity;

        uint8_t pad8;
    } __attribute__((packed));

    struct NamedPartEntry {
        uint16_t cellanimIndex;
        uint16_t arrangementIndex;
        uint16_t partIndex;

        char name[32];
    } __attribute__((packed));

    void Apply(const unsigned char* data, SessionManager::Session* session) {
        const FileHeader* fileHeader = reinterpret_cast<const FileHeader*>(data);

        if (UNLIKELY(memcmp(fileHeader->magic, "TOASTD", 6) != 0)) {
            std::cerr << "[EditorData::Apply] Invalid editor data binary: header magic failed check!\n";
            return;
        }

        if (fileHeader->entryCount == 0)
            return;

        const StandardHeader* currentHeader =
            reinterpret_cast<const StandardHeader*>(data + sizeof(FileHeader));

        auto getPart = [&session](
            uint16_t cellIndex, uint16_t arrngIndex, uint16_t partIndex
        ) -> RvlCellAnim::ArrangementPart* {
            if (cellIndex >= session->cellanims.size()) {
                std::cerr <<
                    "[EditorData::Apply] Invalid editor data binary: oob cellanim index!:\n" <<
                    "   - Cellanim Index: " << cellIndex << '\n';
                return nullptr;
            }

            auto& arrangements = session->cellanims.at(cellIndex).object->arrangements;
            if (arrngIndex >= arrangements.size()) {
                std::cerr <<
                    "[EditorData::Apply] Invalid editor data binary: oob arrangement index!:" <<
                    "   - Cellanim Index: " << cellIndex << '\n' <<
                    "   - Arrangement Index: " << arrngIndex << '\n';
                return nullptr;
            }

            auto& arrangement = arrangements.at(arrngIndex);
            if (partIndex >= arrangement.parts.size()) {
                std::cerr <<
                    "[EditorData::Apply] Invalid editor data binary: oob part index!:" <<
                    "   - Cellanim Index: " << cellIndex << '\n' <<
                    "   - Arrangement Index: " << arrngIndex << '\n' <<
                    "   - Part Index: " << partIndex << '\n';
                return nullptr;
            }

            return &arrangement.parts.at(partIndex);
        };

        for (uint16_t i = 0; i < fileHeader->entryCount; i++) {
            switch (*reinterpret_cast<const uint32_t*>(currentHeader->identifier)) {
                case IDENTIFIER_TO_U32('E', 'L', 'P', 'T'): {
                    const PartLocationEntry* currentEntry = reinterpret_cast<const PartLocationEntry*>(
                        currentHeader + 1
                    );
                    for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                        auto* part = getPart(
                            currentEntry->cellanimIndex,
                            currentEntry->arrangementIndex,
                            currentEntry->partIndex
                        );

                        if (LIKELY(!!part))
                            part->editorLocked = true;
                        else
                            return;

                        currentEntry++;
                    }
                } break;

                case IDENTIFIER_TO_U32('E', 'I', 'P', 'T'): {
                    const PartAlphaEntry* currentEntry = reinterpret_cast<const PartAlphaEntry*>(
                        currentHeader + 1
                    );
                    for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                        auto* part = getPart(
                            currentEntry->cellanimIndex,
                            currentEntry->arrangementIndex,
                            currentEntry->partIndex
                        );

                        if (LIKELY(!!part)) {
                            part->opacity = currentEntry->opacity;
                            part->editorVisible = false;
                        }
                        else
                            return;

                        currentEntry++;
                    }
                } break;

                case IDENTIFIER_TO_U32('A', 'P', 'N', 'T'): {
                    const NamedPartEntry* currentEntry = reinterpret_cast<const NamedPartEntry*>(
                        currentHeader + 1
                    );
                    for (uint16_t j = 0; j < currentHeader->entryCount; j++) {
                        auto* part = getPart(
                            currentEntry->cellanimIndex,
                            currentEntry->arrangementIndex,
                            currentEntry->partIndex
                        );

                        if (LIKELY(!!part))
                            strlcpy(part->editorName, currentEntry->name, 32);
                        else
                            return;

                        currentEntry++;
                    }
                } break;

                default:
                    std::cerr << "[EditorData::Apply] Unimplemented entry type: " <<
                        currentHeader->identifier[0] <<
                        currentHeader->identifier[1] <<
                        currentHeader->identifier[2] <<
                        currentHeader->identifier[3] <<
                    '\n';
                    break;
            }

            if (i + 1 < fileHeader->entryCount)
                currentHeader =
                    reinterpret_cast<const StandardHeader*>(data + currentHeader->nextHeaderOffset);
        }
    }

    std::vector<unsigned char> Write(SessionManager::Session* session) {
        std::vector<unsigned char> result(sizeof(FileHeader));

        FileHeader* fileHeader = reinterpret_cast<FileHeader*>(result.data());
        *fileHeader = FileHeader{};

        std::vector<PartLocationEntry> lockedParts;
        std::vector<PartAlphaEntry> invisibleParts;
        std::vector<NamedPartEntry> namedParts;

        for (uint16_t i = 0; i < session->cellanims.size(); i++) {
            const auto cellanim = session->cellanims.at(i).object;

            for (uint16_t j = 0; j < cellanim->arrangements.size(); j++) {
                const auto& arrangement = cellanim->arrangements.at(j);

                for (uint16_t n = 0; n < arrangement.parts.size(); n++) {
                    const auto& part = arrangement.parts.at(n);

                    if (part.editorLocked)
                        lockedParts.push_back({ i, j, n });
                    if (!part.editorVisible)
                        invisibleParts.push_back({ i, j, n, part.opacity });

                    if (part.editorName[0] != '\0') {
                        NamedPartEntry entry { i, j, n };
                        strlcpy(entry.name, part.editorName, 32);

                        namedParts.push_back(entry);
                    }
                }
            }
        }

        uint32_t writeOffset{ sizeof(FileHeader) };

        if (!invisibleParts.empty()) {
            fileHeader->entryCount++;

            uint32_t endOffset =
                writeOffset + sizeof(StandardHeader) +
                (sizeof(PartAlphaEntry) * invisibleParts.size());

            result.resize(endOffset);

            StandardHeader* header =
                reinterpret_cast<StandardHeader*>(result.data() + writeOffset);
            *header = StandardHeader {
                .identifier = { 'E', 'I', 'P', 'T' },
                .entryCount = (uint16_t)invisibleParts.size(),
                .nextHeaderOffset = endOffset
            };

            writeOffset += sizeof(StandardHeader);

            for (uint16_t i = 0; i < header->entryCount; i++) {
                *reinterpret_cast<PartAlphaEntry*>(
                    result.data() + writeOffset
                ) = invisibleParts.at(i);

                writeOffset += sizeof(PartAlphaEntry);
            }
        }

        if (!lockedParts.empty()) {
            fileHeader->entryCount++;

            uint32_t endOffset =
                writeOffset + sizeof(StandardHeader) +
                (sizeof(PartLocationEntry) * lockedParts.size());

            result.resize(endOffset);

            StandardHeader* header =
                reinterpret_cast<StandardHeader*>(result.data() + writeOffset);
            *header = StandardHeader {
                .identifier = { 'E', 'L', 'P', 'T' },
                .entryCount = (uint16_t)lockedParts.size(),
                .nextHeaderOffset = endOffset
            };

            writeOffset += sizeof(StandardHeader);

            for (uint16_t i = 0; i < header->entryCount; i++) {
                *reinterpret_cast<PartLocationEntry*>(
                    result.data() + writeOffset
                ) = lockedParts.at(i);

                writeOffset += sizeof(PartLocationEntry);
            }
        }

        if (!namedParts.empty()) {
            fileHeader->entryCount++;

            uint32_t endOffset =
                writeOffset + sizeof(StandardHeader) +
                (sizeof(NamedPartEntry) * namedParts.size());

            result.resize(endOffset);

            StandardHeader* header =
                reinterpret_cast<StandardHeader*>(result.data() + writeOffset);
            *header = StandardHeader {
                    .identifier = { 'A', 'P', 'N', 'T' },
                .entryCount = (uint16_t)namedParts.size(),
                .nextHeaderOffset = endOffset
            };

            writeOffset += sizeof(StandardHeader);

            for (uint16_t i = 0; i < header->entryCount; i++) {
                *reinterpret_cast<NamedPartEntry*>(
                    result.data() + writeOffset
                ) = namedParts.at(i);

                writeOffset += sizeof(NamedPartEntry);
            }
        }

        return result;
    }
};

int32_t SessionManager::PushSessionFromCompressedArc(const char* filePath) {
    Session newSession;

    auto __archiveResult = U8::readYaz0U8Archive(filePath);
    if (UNLIKELY(!__archiveResult.has_value())) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenArchive;

        return -1;
    }

    U8::U8ArchiveObject archiveObject = std::move(*__archiveResult);

    if (UNLIKELY(archiveObject.structure.subdirectories.size() < 1)) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_RootDirNotFound;

        return -1;
    }

    auto __tplSearch = U8::findFile("./cellanim.tpl", archiveObject.structure);
    if (UNLIKELY(!__tplSearch.has_value())) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailFindTPL;

        return -1;
    }

    TPL::TPLObject tplObject =
        TPL::TPLObject((*__tplSearch).data.data(), (*__tplSearch).data.size());

    std::vector<const U8::File*> brcadFiles;
    for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
        if (
            file.name.size() >= 6 &&
            file.name.substr(file.name.size() - 6) == ".brcad"
        ) {
            brcadFiles.push_back(&file);
        }
    }

    if (UNLIKELY(brcadFiles.empty())) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_NoBXCADsFound;

        return -1;
    }

    // Sort cellanim files alphabetically for consistency when exporting.
    std::sort(brcadFiles.begin(), brcadFiles.end(), [](const U8::File*& a, const U8::File*& b) {
        return a->name < b->name;
    });

    newSession.cellanims.resize(brcadFiles.size());
    newSession.sheets.resize(tplObject.textures.size());

    // cellanims
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        auto& cellanim = newSession.cellanims.at(i);

        cellanim.name = brcadFiles.at(i)->name;
        cellanim.object =
            std::make_shared<RvlCellAnim::RvlCellAnimObject>(
            RvlCellAnim::RvlCellAnimObject(
                brcadFiles.at(i)->data.data(), brcadFiles.at(i)->data.size()
            ));

        if (UNLIKELY(!cellanim.object->ok)) {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->lastSessionError = SessionOpenError_FailOpenBXCAD;

            return -1;
        }
    }

    // animation names
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        // Find header file
        const U8::File* headerFile{ nullptr };
        std::string targetHeaderName =
            "rcad_" +
            brcadFiles.at(i)->name.substr(0, brcadFiles.at(i)->name.size() - 6) +
            "_labels.h";

        // Find header file
        for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
            if (file.name == targetHeaderName) {
                headerFile = &file;
                continue;
            }
        }

        if (!headerFile)
            continue;

        // Process defines
        std::istringstream stringStream(std::string(headerFile->data.begin(), headerFile->data.end()));
        std::string line;

        while (std::getline(stringStream, line)) {
            if (line.compare(0, 7, "#define") == 0) {
                std::istringstream lineStream(line);
                std::string define, key;
                uint32_t value;

                lineStream >> define >> key >> value;

                size_t commentPos = key.find("//");
                if (commentPos != std::string::npos)
                    key = key.substr(0, commentPos);

                newSession.cellanims.at(i).animNames.insert({ value, key });
            }
        }
    }

    // sheets
    for (unsigned i = 0; i < tplObject.textures.size(); i++) {
        auto& sheet = newSession.sheets.at(i);

        sheet = std::make_shared<Common::Image>(
            tplObject.textures.at(i).width,
            tplObject.textures.at(i).height,
            TPL::LoadTPLTextureIntoGLTexture(tplObject.textures.at(i))
        );

        sheet->tplOutFormat = tplObject.textures.at(i).format;
        sheet->tplColorPalette = tplObject.textures.at(i).palette;
    }

    // internal editor data

    bool dataExpected{ false };
    for (const auto& cd : newSession.cellanims)
        if (cd.object->expectEditorData) {
            dataExpected = true;
            break;
        }

    auto __datSearch = U8::findFile(
        EDITORDATA_RESOURCE_NAME,
        archiveObject.structure.subdirectories[0]
    );
    if (__datSearch.has_value() && dataExpected) {
        EditorData::Apply((*__datSearch).data.data(), &newSession);
    }
    else if (dataExpected) {
        std::cerr << "[SessionManager::PushSessionFromCompressedArc] Supplemental editor data is expected but was not found!\n";

        MtCommandManager::getInstance().enqueueCommand([]() {
            AppState::getInstance().OpenGlobalPopup("###DialogEditorDataExpected");
        }).get();
    }

    newSession.mainPath = filePath;

    std::lock_guard<std::mutex> lock(this->mtx);

    this->sessionList.push_back(std::move(newSession));

    this->lastSessionError = SessionError_None;

    return static_cast<int32_t>(this->sessionList.size() - 1);
}

int32_t SessionManager::PushSessionTraditional(const char* paths[3]) {
    Session newSession;
    newSession.cellanims.resize(1);
    newSession.sheets.resize(1);

    newSession.cellanims.at(0).object = RvlCellAnim::readRvlCellAnimFile(paths[0]);
    if (
        !newSession.cellanims.at(0).object ||
        !newSession.cellanims.at(0).object->ok
    ) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenBXCAD;

        return -1;
    }

    {
        std::filesystem::path filePath(paths[0]);
        newSession.cellanims.at(0).name = filePath.filename().string();
    }

    newSession.sheets.at(0) =
        std::make_shared<Common::Image>(Common::Image());

    newSession.sheets.at(0)->LoadFromFile(paths[1]);

    if (!newSession.sheets.at(0)->texture) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenPNG;

        return -1;
    }

    // animationNames
    auto& animationNames = newSession.cellanims.at(0).animNames;
    {
        std::ifstream headerFile(paths[2]);
        if (!headerFile.is_open()) {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->lastSessionError = SessionOpenError_FailOpenHFile;
            return -1;
        }

        std::string line;
        while (std::getline(headerFile, line)) {
            if (line.compare(0, 7, "#define") == 0) {
                std::istringstream lineStream(line);
                std::string define, key;
                uint32_t value;

                lineStream >> define >> key >> value;

                size_t commentPos = key.find("//");
                if (commentPos != std::string::npos)
                    key = key.substr(0, commentPos);

                animationNames.insert({ value, key });
            }
        }
    }

    newSession.mainPath = paths[0];

    newSession.traditionalMethod = true;
    newSession.pngPath = std::make_unique<std::string>(std::string(paths[1]));
    newSession.headerPath = std::make_unique<std::string>(std::string(paths[2]));

    std::lock_guard<std::mutex> lock(this->mtx);

    this->sessionList.push_back(std::move(newSession));

    this->lastSessionError = SessionError_None;

    return static_cast<int32_t>(this->sessionList.size() - 1);
}

int32_t SessionManager::ExportSessionCompressedArc(Session* session, const char* outPath) {
    std::lock_guard<std::mutex> lock(this->mtx);

    U8::U8ArchiveObject archive;

    { // Serialize all sub-data
        U8::Directory directory(".");

        // BRCAD files
        for (unsigned i = 0; i < session->cellanims.size(); i++) {
            U8::File file(session->cellanims.at(i).name);
            file.data = session->cellanims.at(i).object->Reserialize();

            directory.AddFile(std::move(file));
        }

        // Header files
        for (unsigned i = 0; i < session->cellanims.size(); i++) {
            const auto& map = session->cellanims.at(i).animNames;

            if (map.empty())
                continue; // Don't bother writing the header if there's no
                          // definitions avaliable.

            std::stringstream stream;
            for (auto it = map.begin(); it != map.end(); ++it) {
                stream << "#define " << it->second << " " << std::to_string(it->first);

                if (std::next(it) != map.end())
                    stream << "\n";
            }

            const std::string& cellanimName = session->cellanims.at(i).name;
            U8::File file(
                "rcad_" +
                cellanimName.substr(0, cellanimName.size() - 6) +
                "_labels.h"
            );

            {
                char c;
                while (stream.get(c))
                    file.data.push_back(c);
            }

            directory.AddFile(std::move(file));
        }

        // TPL file
        {
            U8::File file("cellanim.tpl");

            TPL::TPLObject tplObject;

            for (unsigned i = 0; i < session->sheets.size(); i++) {
                auto tplTexture = session->sheets.at(i)->ExportToTPLTexture();

                if (UNLIKELY(!tplTexture.has_value())) {
                    this->lastSessionError = SessionOutError_FailTPLTextureExport;
                    return -1;
                }

                tplObject.textures.push_back(std::move(*tplTexture));
            }

            file.data = tplObject.Reserialize();

            directory.AddFile(file);
        }

        // supplemental editor data
        {
            U8::File file(EDITORDATA_RESOURCE_NAME);

            file.data = EditorData::Write(session);

            directory.AddFile(file);
        }

        directory.SortAlphabetically();

        archive.structure.AddDirectory(directory);
    }

    auto archiveRaw = archive.Reserialize();
    auto compressedArchive = Yaz0::compress(archiveRaw.data(), archiveRaw.size());

    if (UNLIKELY(!compressedArchive.has_value())) {
        this->lastSessionError = SessionOutError_ZlibError;
        return -1;
    }

    GET_CONFIG_MANAGER;
    if (
        configManager.getConfig().backupBehaviour != BackupBehaviour_None &&
        !Common::SaveBackupFile(
            outPath,

            configManager.getConfig().backupBehaviour == BackupBehaviour_SaveOnce
        )
    )
        std::cerr << "[SessionManager::ExportSessionCompressedArc] Failed to save backup of file!\n";

    std::ofstream file(outPath, std::ios::binary);
    if (LIKELY(file.is_open())) {
        file.write(
            reinterpret_cast<const char*>((*compressedArchive).data()),
            (*compressedArchive).size()
        );

        file.close();
    }
    else {
        this->lastSessionError = SessionOutError_FailOpenFile;
        return -1;
    }

    session->modified = false;

    return 0;
}

void SessionManager::FreeSessionIndex(int32_t index) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (
        UNLIKELY(index >= this->sessionList.size()) ||
        UNLIKELY(index < 0)
    )
        return;

    auto it = this->sessionList.begin() + index;
    this->sessionList.erase(it);

    this->currentSession =
        std::clamp<int32_t>(this->currentSession, -1, this->sessionList.size() - 1);
}

void SessionManager::FreeAllSessions() {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->sessionList.clear();

    this->currentSession = -1;
}

void SessionManager::SessionChanged() {
    std::lock_guard<std::mutex> lock(this->mtx);

    this->currentSession =
        std::clamp<int32_t>(this->currentSession, -1, this->sessionList.size() - 1);

    if (UNLIKELY(this->currentSession < 0))
        return;

    GET_APP_STATE;
    GET_ANIMATABLE;

    Common::deleteIfNotNullptr(globalAnimatable);

    globalAnimatable = new Animatable(
        this->getCurrentSession()->getCellanimObject(),
        this->getCurrentSession()->getCellanimSheet()
    );

    appState.selectedAnimation = std::clamp<uint16_t>(
        appState.selectedAnimation,
        0,
        globalAnimatable->cellanim->animations.size() - 1
    );

    globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);

    if (appState.getArrangementMode()) {
        appState.controlKey.arrangementIndex = std::clamp<uint16_t>(
            appState.controlKey.arrangementIndex,
            0,
            globalAnimatable->cellanim->arrangements.size() - 1
        );

        PlayerManager::getInstance().setAnimating(false);
        globalAnimatable->overrideAnimationKey(&appState.controlKey);
    }

    appState.correctSelectedPart();
}
