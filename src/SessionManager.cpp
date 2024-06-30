#include "SessionManager.hpp"

#include "archive/U8.hpp"
#include "compression/Yaz0.hpp"

#include "texture/TPL.hpp"

#include <sstream>
#include <fstream>

#include "AppState.hpp"

#include "common.hpp"

#include <filesystem>

#include "ConfigManager.hpp"

int32_t SessionManager::PushSessionFromCompressedArc(const char* filePath) {
    Session newSession;

    auto TMParchiveResult = U8::readYaz0U8Archive(filePath);
    if (!TMParchiveResult.has_value()) {
        this->lastSessionError = SessionOpenError_FailOpenArchive;
        return -1;
    }

    U8::U8ArchiveObject archiveObject = std::move(*TMParchiveResult);

    if (archiveObject.structure.subdirectories.size() < 1) {
        this->lastSessionError = SessionOpenError_RootDirNotFound;
        return -1;
    }

    auto TMPtplSearch = U8::findFile("./cellanim.tpl", archiveObject.structure);
    if (!TMPtplSearch.has_value()) {
        this->lastSessionError = SessionOpenError_FailFindTPL;
        return -1;
    }

    U8::File tplFile = std::move(*TMPtplSearch);
    TPL::TPLObject tplObject =
        TPL::TPLObject(tplFile.data.data(), tplFile.data.size());

    std::vector<const U8::File*> brcadFiles;
    for (const auto& file : archiveObject.structure.subdirectories.at(0).files) {
        if (file.name.size() >= 6 && file.name.substr(file.name.size() - 6) == ".brcad") {
            brcadFiles.push_back(&file);
        }
    }

    if (brcadFiles.size() < 1) {
        this->lastSessionError = SessionOpenError_NoBXCADsFound;
        return -1;
    }

    newSession.animationNames.resize(brcadFiles.size());
    newSession.cellanims.resize(brcadFiles.size());
    newSession.cellanimSheets.resize(tplObject.textures.size());

    // animationNames
    for (uint16_t i = 0; i < brcadFiles.size(); i++) {
        // Find header file
        const U8::File* headerFile{ nullptr };
        std::string targetHeaderName =
            "rcad_" +
            brcadFiles.at(i)->name.substr(0, brcadFiles.at(i)->name.size() - 6) +
            "_labels.h";

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
                uint16_t value;

                lineStream >> define >> key >> value;

                size_t commentPos = key.find("//");
                if (commentPos != std::string::npos) {
                    key = key.substr(0, commentPos);
                }

                if (!newSession.animationNames.at(i))
                    newSession.animationNames.at(i) = new std::unordered_map<uint16_t, std::string>;

                newSession.animationNames.at(i)->insert({ value, key });
            }
        }
    }

    // cellanims
    for (uint16_t i = 0; i < brcadFiles.size(); i++) {
        newSession.cellanims.at(i) = new RvlCellAnim::RvlCellAnimObject(
            brcadFiles.at(i)->data.data(), brcadFiles.at(i)->data.size()
        );
        if (!newSession.cellanims.at(i)->ok) {
            this->lastSessionError = SessionOpenError_FailOpenBXCAD;
            return -1;
        }
    }

    // cellanimSheets
    for (uint16_t i = 0; i < tplObject.textures.size(); i++) {
        newSession.cellanimSheets.at(i) = new Common::Image(
            tplObject.textures.at(i).width,
            tplObject.textures.at(i).height,
            TPL::LoadTPLTextureIntoGLTexture(tplObject.textures.at(i))
        );
        newSession.cellanimSheets.at(i)->tplOutFormat = tplObject.textures.at(i).format;
        newSession.cellanimSheets.at(i)->tplColorPalette = tplObject.textures.at(i).palette;
    }

    newSession.open = true;
    newSession.mainPath = filePath;

    for (auto brcad : brcadFiles)
        newSession.cellNames.push_back(brcad->name);

    this->sessionList.push_back(std::move(newSession));

    this->lastSessionError = SessionOpenError_None;

    return static_cast<int32_t>(this->sessionList.size() - 1);
}

int32_t SessionManager::PushSessionTraditional(const char* paths[3]) {
    Session newSession;

    RvlCellAnim::RvlCellAnimObject* cellanim = RvlCellAnim::ObjectFromFile(paths[0]);
    if (!cellanim || !cellanim->ok) {
        Common::deleteIfNotNullptr(cellanim, false);

        this->lastSessionError = SessionOpenError_FailOpenBXCAD;
        return -1;
    }

    Common::Image* image = new Common::Image;
    image->LoadFromFile(paths[1]);

    if (!image->texture) {
        Common::deleteIfNotNullptr(cellanim, false);
        delete image;

        this->lastSessionError = SessionOpenError_FailOpenPNG;
        return -1;
    }

    // animationNames
    std::unordered_map<uint16_t, std::string>* animationNames =
        new std::unordered_map<uint16_t, std::string>;    
    {
        std::ifstream headerFile(paths[2]);
        if (!headerFile.is_open()) {
            this->lastSessionError = SessionOpenError_FailOpenHFile;
            return -1;
        }

        std::string line;
        while (std::getline(headerFile, line)) {
            if (line.compare(0, 7, "#define") == 0) {
                std::istringstream lineStream(line);
                std::string define, key;
                uint16_t value;

                lineStream >> define >> key >> value;

                size_t commentPos = key.find("//");
                if (commentPos != std::string::npos) {
                    key = key.substr(0, commentPos);
                }

                animationNames->insert({ value, key });
            }
        }
    }

    newSession.open = true;
    newSession.mainPath = paths[0];

    newSession.traditionalMethod = true;

    newSession.pngPath = new std::string(paths[1]);
    newSession.headerPath = new std::string(paths[2]);

    newSession.animationNames.push_back(animationNames);
    newSession.cellanims.push_back(cellanim);
    newSession.cellanimSheets.push_back(image);

    std::filesystem::path filePath(paths[0]);
    newSession.cellNames.push_back(filePath.filename().string());

    this->sessionList.push_back(std::move(newSession));

    this->lastSessionError = SessionOpenError_None;

    return static_cast<int32_t>(this->sessionList.size() - 1);
}

int32_t SessionManager::ExportSessionCompressedArc(Session* session, const char* outPath) {
    U8::U8ArchiveObject archive;

    { // Serialize all sub-data
        U8::Directory directory(".");
        
        // BRCAD files
        for (uint16_t i = 0; i < session->cellanims.size(); i++) {
            U8::File file(session->cellNames.at(i));

            file.data = session->cellanims.at(i)->Reserialize();

            directory.AddFile(file);
        }

        // H files
        for (uint16_t i = 0; i < session->animationNames.size(); i++) {
            std::stringstream stream;

            auto map = session->animationNames.at(i);
            for (auto it = map->begin(); it != map->end(); ++it) {
                stream << "#define " << it->second << " " << std::to_string(it->first);
                
                if (std::next(it) != map->end())
                    stream << "\n";
            }
        
            U8::File file(
                "rcad_" +
                session->cellNames.at(i).substr(0, session->cellNames.at(i).size() - 6) +
                "_labels.h"
            );

            char c;
            while (stream.get(c))
                file.data.push_back(c);

            directory.AddFile(file);
        }

        // TPL file
        {
            U8::File file("cellanim.tpl");

            TPL::TPLObject tplObject;

            for (uint32_t i = 0; i < session->cellanimSheets.size(); i++) {
                auto tplTexture = session->cellanimSheets.at(i)->ExportToTPLTexture();

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

void SessionManager::ClearSessionPtr(Session* session) {
    for (auto animationNameList : session->animationNames)
        delete animationNameList;
    for (auto cellanim : session->cellanims)
        delete cellanim;
    for (auto cellanimSheet : session->cellanimSheets) {
        cellanimSheet->FreeTexture();
        delete cellanimSheet;
    }

    session->animationNames.clear();
    session->cellanims.clear();
    session->cellanimSheets.clear();
    session->cellNames.clear();

    session->cellIndex = 0;

    Common::deleteIfNotNullptr(session->pngPath);
    Common::deleteIfNotNullptr(session->headerPath);

    session->open = false;
}

void SessionManager::FreeSessionIndex(int32_t index) {
    if (index >= this->sessionList.size() || index < 0)
        return;

    if (this->sessionList.at(index).open)
        this->ClearSessionPtr(&this->sessionList.at(index));

    std::deque<Session>::iterator it = this->sessionList.begin() + index;
    this->sessionList.erase(it);

    // Correct currentSession
    if (this->sessionList.empty())
        this->currentSession = -1;
    else if (static_cast<size_t>(this->currentSession) >= this->sessionList.size())
        this->currentSession = static_cast<int32_t>(this->sessionList.size() - 1);
}

void SessionManager::FreeAllSessions() {
    for (uint32_t i = 0; i < this->sessionList.size(); i++)
        this->FreeSessionIndex(i);
}

void SessionManager::SessionChanged() {
    if (this->getCurrentSession() && !this->getCurrentSession()->open)
        this->currentSession = -1;

    if (this->currentSession >= 0) {
        GET_APP_STATE;
        GET_ANIMATABLE;

        Common::deleteIfNotNullptr(globalAnimatable);

        globalAnimatable = new Animatable(
            this->getCurrentSession()->getCellanim(),
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