#include "SessionManager.hpp"

#include "archive/U8.hpp"
#include "compression/Yaz0.hpp"

#include "texture/TPL.hpp"

#include <sstream>
#include <fstream>

#include "AppState.hpp"

#include "common.hpp"

#include <filesystem>

#include <thread>

#include "ConfigManager.hpp"

int32_t SessionManager::PushSessionFromCompressedArc(const char* filePath) {
    Session newSession;

    auto __archiveResult = U8::readYaz0U8Archive(filePath);
    if (!__archiveResult.has_value()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailOpenArchive;

        return -1;
    }

    U8::U8ArchiveObject archiveObject = std::move(*__archiveResult);

    if (archiveObject.structure.subdirectories.size() < 1) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_RootDirNotFound;

        return -1;
    }

    auto __tplSearch = U8::findFile("./cellanim.tpl", archiveObject.structure);
    if (!__tplSearch.has_value()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_FailFindTPL;
        
        return -1;
    }

    TPL::TPLObject tplObject =
        TPL::TPLObject((*__tplSearch).data.data(), (*__tplSearch).data.size());

    std::vector<const U8::File*> brcadFiles;
    for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
        if (file.name.size() >= 6 && file.name.substr(file.name.size() - 6) == ".brcad") {
            brcadFiles.push_back(&file);
        }
    }

    if (brcadFiles.empty()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->lastSessionError = SessionOpenError_NoBXCADsFound;

        return -1;
    }

    newSession.cellanims.resize(brcadFiles.size());
    newSession.sheets.resize(tplObject.textures.size());

    // cellanims
    for (uint32_t i = 0; i < brcadFiles.size(); i++) {
        auto& cellanim = newSession.cellanims.at(i);

        cellanim.name = brcadFiles.at(i)->name;
        cellanim.object =
            std::make_shared<RvlCellAnim::RvlCellAnimObject>(
            RvlCellAnim::RvlCellAnimObject(
                brcadFiles.at(i)->data.data(), brcadFiles.at(i)->data.size()
            ));

        if (!cellanim.object->ok) {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->lastSessionError = SessionOpenError_FailOpenBXCAD;

            return -1;
        }
    }

    // animation names
    for (uint32_t i = 0; i < brcadFiles.size(); i++) {
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
    for (uint32_t i = 0; i < tplObject.textures.size(); i++) {
        auto& sheet = newSession.sheets.at(i);

        sheet = 
            std::make_shared<Common::Image>(
            Common::Image(
                tplObject.textures.at(i).width,
                tplObject.textures.at(i).height,
                TPL::LoadTPLTextureIntoGLTexture(tplObject.textures.at(i))
            ));
        
        sheet->tplOutFormat = tplObject.textures.at(i).format;
        sheet->tplColorPalette = tplObject.textures.at(i).palette;
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
        for (uint32_t i = 0; i < session->cellanims.size(); i++) {
            U8::File file(session->cellanims.at(i).name);
            file.data = session->cellanims.at(i).object->Reserialize();

            directory.AddFile(std::move(file));
        }

        // Header files
        for (uint32_t i = 0; i < session->cellanims.size(); i++) {
            std::stringstream stream;

            const auto& map = session->cellanims.at(i).animNames;
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

            for (uint32_t i = 0; i < session->sheets.size(); i++) {
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

        directory.SortAlphabetically();

        archive.structure.AddDirectory(directory);
    }

    auto archiveRaw = archive.Reserialize();
    auto compressedArchive = Yaz0::compress(archiveRaw.data(), archiveRaw.size());

    if (!compressedArchive.has_value()) {
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
    if (file.is_open()) {
        file.write(
            reinterpret_cast<const char*>((*compressedArchive).data()),
            (*compressedArchive).size()
        );
        
        file.close();
    } else {
        this->lastSessionError = SessionOutError_FailOpenFile;
        return -1;
    }

    session->modified = false;

    return 0;
}

void SessionManager::FreeSessionIndex(int32_t index) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (index >= this->sessionList.size() || index < 0)
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

    if (this->currentSession >= 0) {
        GET_APP_STATE;
        GET_ANIMATABLE;

        Common::deleteIfNotNullptr(globalAnimatable);

        globalAnimatable = new Animatable(
            this->getCurrentSession()->getCellanimObject(),
            this->getCurrentSession()->getCellanimSheet()
        );

        if (appState.getArrangementMode()) {
            appState.controlKey.arrangementIndex = std::clamp<uint16_t>(
                appState.controlKey.arrangementIndex,
                0,
                globalAnimatable->cellanim->arrangements.size() - 1
            );

            appState.playerState.ToggleAnimating(false);
            globalAnimatable->overrideAnimationKey(&appState.controlKey);
        } else {
            appState.selectedAnimation = std::clamp<uint16_t>(
                appState.selectedAnimation,
                0,
                globalAnimatable->cellanim->animations.size() - 1
            );

            globalAnimatable->setAnimationFromIndex(appState.selectedAnimation);
        }
        appState.playerState.updateSetFrameCount();

        appState.selectedPart = std::clamp<int32_t>(
            appState.selectedPart,
            -1,
            globalAnimatable->getCurrentArrangement()->parts.size() - 1
        );
    }
}