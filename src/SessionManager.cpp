#include "SessionManager.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <sstream>
#include <fstream>

#include <filesystem>

#include "anim/CellAnim.hpp"

#include "archive/U8Archive.hpp"
#include "compression/Yaz0.hpp"

#include "texture/TPL.hpp"

#include "EditorDataPackage.hpp"

#include "ConfigManager.hpp"
#include "PlayerManager.hpp"
#include "MainThreadTaskManager.hpp"

#include "AppState.hpp"

#include "files.hpp"

#include "common.hpp"

void Session::addCommand(std::shared_ptr<BaseCommand> command) {
    command->Execute();
    if (this->undoQueue.size() == COMMANDS_MAX)
        this->undoQueue.pop_front();

    this->undoQueue.push_back(command);
    this->redoQueue.clear();
}

void Session::undo() {
    if (this->undoQueue.empty())
        return;

    auto command = undoQueue.back();
    this->undoQueue.pop_back();

    command->Rollback();

    this->redoQueue.push_back(command);

    this->modified = true;
}

void Session::redo() {
    if (this->redoQueue.empty())
        return;

    auto command = redoQueue.back();
    this->redoQueue.pop_back();

    command->Execute();

    if (this->undoQueue.size() == COMMANDS_MAX)
        this->undoQueue.pop_front();

    this->undoQueue.push_back(command);

    this->modified = true;
}

void Session::setCurrentCellanimIndex(unsigned index) {
    if (index >= this->cellanims.size())
        return;

    this->currentCellanim = index;
    this->sheets->setBaseTextureIndex(
        this->cellanims.at(index).object->sheetIndex
    );

    PlayerManager::getInstance().correctState();
}

void SessionManager::setCurrentSessionIndex(int sessionIndex) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (sessionIndex >= this->sessions.size())
        return;
    
    this->currentSessionIndex = sessionIndex;

    PlayerManager::getInstance().correctState();
}

int SessionManager::CreateSession(const char* filePath) {
    if (!Files::doesFileExist(filePath)) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_FileDoesNotExist;

        return -1;
    }

    Session newSession;

    auto __archiveResult = U8Archive::readYaz0U8Archive(filePath);
    if (!__archiveResult.has_value()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_FailOpenArchive;

        return -1;
    }

    U8Archive::U8ArchiveObject archiveObject = std::move(*__archiveResult);

    auto rootDirIt = std::find_if(
        archiveObject.structure.subdirectories.begin(),
        archiveObject.structure.subdirectories.end(),
        [](const U8Archive::Directory& dir) { return dir.name == "."; }
    );

    if (rootDirIt == archiveObject.structure.subdirectories.end()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_RootDirNotFound;

        return -1;
    }

    // Every layout archive has the directory "./blyt". If this directory exists
    // we should throw an error
    auto blytDirIt = std::find_if(
        rootDirIt->subdirectories.begin(),
        rootDirIt->subdirectories.end(),
        [](const U8Archive::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != rootDirIt->subdirectories.end()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_LayoutArchive;

        return -1;
    }

    U8Archive::File* __tplSearch = U8Archive::findFile("cellanim.tpl", *rootDirIt);
    if (!__tplSearch) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_FailFindTPL;

        return -1;
    }

    TPL::TPLObject tplObject =
        TPL::TPLObject(__tplSearch->data.data(), __tplSearch->data.size());

    if (!tplObject.ok) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_FailOpenTPL;

        return -1;
    }

    std::vector<const U8Archive::File*> brcadFiles;
    for (const auto& file : rootDirIt->files) {
        if (
            file.name.size() >= 6 &&
            file.name.substr(file.name.size() - 6) == ".brcad"
        )
            brcadFiles.push_back(&file);
    }

    if (brcadFiles.empty()) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_NoBXCADsFound;

        return -1;
    }

    // Sort cellanim files alphabetically for consistency when exporting.
    std::sort(brcadFiles.begin(), brcadFiles.end(), [](const U8Archive::File*& a, const U8Archive::File*& b) {
        return a->name < b->name;
    });

    newSession.cellanims.resize(brcadFiles.size());

    newSession.sheets->getVector().reserve(tplObject.textures.size());

    // Cellanims
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        auto& cellanim = newSession.cellanims[i];
        const auto* file = brcadFiles[i];

        cellanim.name = file->name;
        cellanim.object =
            std::make_shared<CellAnim::CellAnimObject>(
                file->data.data(), file->data.size()
            );

        if (!cellanim.object->isInitialized()) {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->currentError = OpenError_FailOpenBXCAD;

            return -1;
        }
    }

    // Headers (animation names)
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        // Find header file
        const U8Archive::File* headerFile { nullptr };

        const U8Archive::File* brcadFile = brcadFiles[i];

        char targetHeaderName[128];
        snprintf(
            targetHeaderName, sizeof(targetHeaderName),
            "rcad_%.*s_labels.h",
            static_cast<int>(brcadFile->name.size() - STR_LIT_LEN(".brcad")),
            brcadFile->name.c_str()
        );

        // Find header file
        for (const auto& file : rootDirIt->files) {
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

                auto& animations = newSession.cellanims[i].object->animations;
                if (value < animations.size())
                    newSession.cellanims[i].object->animations.at(value).name = key;
            }
        }
    }

    // Sheets
    // Note: this is wrapped in a MainThreadTask since we need to get access to the
    //       GL context to create GPU textures. This can be outside of a MainThreadTask
    //       but then it would use multiple MainThreadTasks instead of just one.
    MainThreadTaskManager::getInstance().QueueTask([&tplObject, &newSession]() {
        for (unsigned i = 0; i < tplObject.textures.size(); i++) {
            auto& texture = tplObject.textures[i];

            std::shared_ptr<Texture> sheet = std::make_shared<Texture>(
                texture.width, texture.height,
                texture.createGPUTexture()
            );
            sheet->setTPLOutputFormat(texture.format);

            newSession.sheets->addTexture(std::move(sheet));
        }
    }).get();

    // Editor data
    {
        U8Archive::File* tedSearch = U8Archive::findFile(TED_ARC_FILENAME, *rootDirIt);
        if (tedSearch)
            TedApply(tedSearch->data.data(), newSession);
        else {
            U8Archive::File* datSearch = U8Archive::findFile(TED_ARC_FILENAME_OLD, *rootDirIt);
            if (datSearch)
                TedApply(datSearch->data.data(), newSession);
        }
    }

    newSession.resourcePath = filePath;
    newSession.setCurrentCellanimIndex(0);

    std::lock_guard<std::mutex> lock(this->mtx);

    this->sessions.push_back(std::move(newSession));

    this->currentError = Error_None;

    return static_cast<int>(this->sessions.size() - 1);
}

bool SessionManager::ExportSession(unsigned sessionIndex, const char* dstFilePath) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (sessionIndex >= this->sessions.size())
        return false;
    
    auto& session = this->sessions[sessionIndex];

    if (dstFilePath == nullptr)
        dstFilePath = session.resourcePath.c_str();

    U8Archive::U8ArchiveObject archive;

    { // Serialize all sub-data
        U8Archive::Directory directory(".");

        // BRCAD files
        for (unsigned i = 0; i < session.cellanims.size(); i++) {
            U8Archive::File file(session.cellanims[i].name);
            file.data = session.cellanims[i].object->Serialize();

            directory.AddFile(std::move(file));
        }

        // Header files
        for (unsigned i = 0; i < session.cellanims.size(); i++) {
            std::ostringstream stream;
            for (unsigned j = 0; j < session.cellanims[i].object->animations.size(); j++) {
                const auto& animation = session.cellanims[i].object->animations[j];
                if (animation.name.empty())
                    continue;

                stream << "#define " << animation.name << '\t' << std::to_string(j) << "\t// (null)\n";
            }

            const std::string& cellanimName = session.cellanims[i].name;
            U8Archive::File file(
                "rcad_" +
                cellanimName.substr(0, cellanimName.size() - STR_LIT_LEN(".brcad")) +
                "_labels.h"
            );

            const std::string str = stream.str();
            file.data.insert(file.data.end(), str.begin(), str.end());

            directory.AddFile(std::move(file));
        }

        // TPL file
        {
            U8Archive::File file("cellanim.tpl");

            TPL::TPLObject tplObject;

            SessionManager::Error error { Error_None };
            MainThreadTaskManager::getInstance().QueueTask([&session, &tplObject, &error]() {
                for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
                    auto tplTexture = session.sheets->getTextureByIndex(i)->TPLTexture();
                    if (!tplTexture.has_value()) {
                        error = OutError_FailTPLTextureExport;
                        return;
                    }

                    tplObject.textures.push_back(std::move(*tplTexture));
                }
            }).get();

            if (error != Error_None) {
                this->currentError = OutError_FailTPLTextureExport;
                return -1;
            }

            file.data = tplObject.Serialize();

            directory.AddFile(std::move(file));
        }

        // TED file
        {
            U8Archive::File file(TED_ARC_FILENAME);

            TedWriteState* state = TedCreateWriteState(session);
                file.data.resize(TedPrepareWrite(state));
                TedWrite(state, file.data.data());
            TedDestroyWriteState(state);

            directory.AddFile(std::move(file));
        }

        directory.SortAlphabetically();

        archive.structure.AddDirectory(std::move(directory));
    }

    auto archiveRaw = archive.Serialize();

    const ConfigManager& configManager = ConfigManager::getInstance();

    auto compressedArchive = Yaz0::compress(
        archiveRaw.data(), archiveRaw.size(),
        configManager.getConfig().compressionLevel
    );

    if (!compressedArchive.has_value()) {
        this->currentError = OutError_ZlibError;
        return -1;
    }

    if (configManager.getConfig().backupBehaviour != BackupBehaviour_None) {
        bool backedUp = Files::BackupFile(
            dstFilePath,
            configManager.getConfig().backupBehaviour == BackupBehaviour_SaveOnce
        );

        if (!backedUp)
            std::cerr << "[SessionManager::ExportSessionCompressedArc] Failed to save backup of file!\n";
    }

    std::ofstream file(dstFilePath, std::ios::binary);
    if (file.is_open()) {
        file.write(
            reinterpret_cast<const char*>(compressedArchive->data()),
            compressedArchive->size()
        );

        file.close();
    }
    else {
        this->currentError = OutError_FailOpenFile;
        return -1;
    }

    session.modified = false;

    return 0;
}

void SessionManager::RemoveSession(unsigned sessionIndex) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (sessionIndex >= this->sessions.size())
        return;

    auto sessionIt = this->sessions.begin() + sessionIndex;
    this->sessions.erase(sessionIt);

    if (this->sessions.empty())
        this->currentSessionIndex = -1;
    else {
        this->currentSessionIndex = std::min(
            this->currentSessionIndex,
            static_cast<int>(this->sessions.size()) - 1
        );

        PlayerManager::getInstance().correctState();
    }
}

void SessionManager::RemoveAllSessions() {
    std::lock_guard<std::mutex> lock(this->mtx);

    this->sessions.clear();
    this->currentSessionIndex = -1;
}
