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

#include "EditorDataPackage.hpp"

#include "ConfigManager.hpp"
#include "PlayerManager.hpp"
#include "MtCommandManager.hpp"

#include "AppState.hpp"

#include "common.hpp"

int SessionManager::PushSessionFromCompressedArc(const char* filePath) {
    Session newSession;

    auto __archiveResult = U8::readYaz0U8Archive(filePath);
    if (!__archiveResult.has_value()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenArchive;

        return -1;
    }

    U8::U8ArchiveObject archiveObject = std::move(*__archiveResult);

    if (archiveObject.structure.subdirectories.empty()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_RootDirNotFound;

        return -1;
    }

    U8::File* __tplSearch = U8::findFile("./cellanim.tpl", archiveObject.structure);
    if (!__tplSearch) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailFindTPL;

        return -1;
    }

    TPL::TPLObject tplObject =
        TPL::TPLObject(__tplSearch->data.data(), __tplSearch->data.size());

    if (!tplObject.ok) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenTPL;

        return -1;
    }

    std::vector<const U8::File*> brcadFiles;
    for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
        if (
            file.name.size() >= 6 &&
            file.name.substr(file.name.size() - 6) == ".brcad"
        ) {
            brcadFiles.push_back(&file);
        }
    }

    if (brcadFiles.empty()) {
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
        auto& cellanim = newSession.cellanims[i];
        const auto* file = brcadFiles[i];

        cellanim.name = file->name;
        cellanim.object =
            std::make_shared<RvlCellAnim::RvlCellAnimObject>(
                file->data.data(), file->data.size()
            );

        if (!cellanim.object->ok) {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->lastSessionError = SessionOpenError_FailOpenBXCAD;

            return -1;
        }
    }

    // animation names
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        // Find header file
        const U8::File* headerFile{ nullptr };

        const U8::File* brcadFile = brcadFiles[i];

        char targetHeaderName[128];
        snprintf(
            targetHeaderName, sizeof(targetHeaderName),
            "rcad_%.*s_labels.h",
            static_cast<int>(brcadFile->name.size() - 6),
            brcadFile->name.c_str()
        );

        // Find header file
        for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
            if (strcmp(file.name.c_str(), targetHeaderName) == 0) {
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
                unsigned value;

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
        auto& sheet = newSession.sheets[i];

        auto& texture = tplObject.textures[i];

        sheet = std::make_shared<Common::Image>(
            texture.width, texture.height,
            TPL::LoadTPLTextureIntoGLTexture(texture)
        );

        sheet->tplOutFormat = tplObject.textures.at(i).format;
        sheet->tplColorPalette = tplObject.textures.at(i).palette;
    }

    // internal editor data
    {
        U8::File* tedSearch = U8::findFile("./" TED_ARC_FILENAME, archiveObject.structure);
        if (tedSearch)
            TedApply(tedSearch->data.data(), &newSession);
        else {
            U8::File* datSearch = U8::findFile("./" TED_ARC_FILENAME_OLD, archiveObject.structure);
            if (datSearch)
                TedApply(datSearch->data.data(), &newSession);
        }
    }

    newSession.mainPath = filePath;

    std::lock_guard<std::mutex> lock(this->mtx);

    this->sessionList.push_back(std::move(newSession));

    this->lastSessionError = SessionError_None;

    return static_cast<int>(this->sessionList.size() - 1);
}

int SessionManager::PushSessionTraditional(const char* paths[3]) {
    Session newSession;

    newSession.cellanims.resize(1);
    auto& cellanim = newSession.cellanims[0];

    newSession.sheets.resize(1);
    auto& sheet = newSession.sheets[0];

    cellanim.object = RvlCellAnim::readRvlCellAnimFile(paths[0]);
    if (
        !cellanim.object ||
        !cellanim.object->ok
    ) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenBXCAD;

        return -1;
    }

    {
        std::filesystem::path filePath(paths[0]);
        cellanim.name = filePath.filename().string();
    }

    sheet = std::make_shared<Common::Image>();
    sheet->LoadFromFile(paths[1]);

    if (!sheet->texture) {
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
                unsigned value;

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

    return static_cast<int>(this->sessionList.size() - 1);
}

int SessionManager::ExportSessionCompressedArc(Session* session, const char* outPath) {
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

            const std::string str = stream.str();
            file.data.insert(file.data.end(), str.begin(), str.end());

            directory.AddFile(std::move(file));
        }

        // TPL file
        {
            U8::File file("cellanim.tpl");

            TPL::TPLObject tplObject;

            for (unsigned i = 0; i < session->sheets.size(); i++) {
                auto tplTexture = session->sheets.at(i)->ExportToTPLTexture();

                if (!tplTexture.has_value()) {
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
            U8::File file(TED_ARC_FILENAME);

            file.data.resize(TedPrecomputeSize(session));
            TedWrite(session, file.data.data());

            directory.AddFile(file);
        }

        directory.SortAlphabetically();

        archive.structure.AddDirectory(directory);
    }

    auto archiveRaw = archive.Reserialize();

    GET_CONFIG_MANAGER;

    auto compressedArchive = Yaz0::compress(
        archiveRaw.data(), archiveRaw.size(),
        configManager.getConfig().compressionLevel
    );

    if (!compressedArchive.has_value()) {
        this->lastSessionError = SessionOutError_ZlibError;
        return -1;
    }

    if (
        configManager.getConfig().backupBehaviour != BackupBehaviour_None &&
        !Common::SaveBackupFile(
            outPath,

            configManager.getConfig().backupBehaviour == BackupBehaviour_SaveOnce
        )
    )
        std::cerr << "[SessionManager::ExportSessionCompressedArc] Failed to save backup of file!\n";

    std::ofstream file(outPath, std::ios::binary);
    if (file.is_open()) {
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

void SessionManager::SessionChanged() {
    std::lock_guard<std::mutex> lock(this->mtx);

    this->currentSession =
        std::clamp<int>(this->currentSession, -1, this->sessionList.size() - 1);

    if (this->currentSession < 0)
        return;

    GET_APP_STATE;
    GET_ANIMATABLE;

    Common::deleteIfNotNullptr(globalAnimatable, false);

    globalAnimatable = new Animatable(
        this->sessionList[this->currentSession].getCellanimObject(),
        this->sessionList[this->currentSession].getCellanimSheet()
    );

    appState.selectedAnimation = std::min<unsigned>(
        appState.selectedAnimation,
        globalAnimatable->cellanim->animations.size() - 1
    );

    globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);

    if (appState.getArrangementMode()) {
        appState.controlKey.arrangementIndex = std::min<uint16_t>(
            appState.controlKey.arrangementIndex,
            globalAnimatable->cellanim->arrangements.size() - 1
        );

        PlayerManager::getInstance().setPlaying(false);
        globalAnimatable->overrideAnimationKey(&appState.controlKey);
    }

    appState.correctSelectedParts();
}

void SessionManager::FreeSessionIndex(int index) {
    std::lock_guard<std::mutex> lock(this->mtx);

    auto it = this->sessionList.begin() + index;
    this->sessionList.erase(it);

    this->currentSession =
        std::clamp<int>(this->currentSession, -1, this->sessionList.size() - 1);
}

void SessionManager::FreeAllSessions() {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->sessionList.clear();

    this->currentSession = -1;
}
