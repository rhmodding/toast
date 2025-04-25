#include "SessionManager.hpp"

#include <cstdio>
#include <cstring>

#include <sstream>
#include <fstream>

#include "Logging.hpp"

#include <algorithm>

#include "cellanim/CellAnim.hpp"

#include "archive/U8Archive.hpp"
#include "archive/SARC.hpp"

#include "compression/Yaz0.hpp"
#include "compression/NZlib.hpp"

#include "texture/TPL.hpp"
#include "texture/CTPK.hpp"

#include "EditorDataPackage.hpp"

#include "ConfigManager.hpp"
#include "PlayerManager.hpp"
#include "MainThreadTaskManager.hpp"

#include "Files.hpp"

#include "common.hpp"

void SessionManager::setCurrentSessionIndex(int sessionIndex) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (sessionIndex >= this->sessions.size())
        return;

    this->currentSessionIndex = sessionIndex;

    PlayerManager::getInstance().correctState();

    Logging::info << "[SessionManager::setCurrentSessionIndex] Selected session no. " << sessionIndex+1 << '.' << std::endl;
}

static SessionManager::Error InitRvlSession(
    Session& session, U8Archive::U8ArchiveObject& archive
) {
    auto rootDirIt = std::find_if(
        archive.structure.subdirectories.begin(),
        archive.structure.subdirectories.end(),
        [](const U8Archive::Directory& dir) { return dir.name == "."; }
    );

    if (rootDirIt == archive.structure.subdirectories.end()) {
        return SessionManager::OpenError_RootDirNotFound;
    }

    // Every layout archive has the directory "./blyt". If this directory exists
    // we should throw an error
    auto blytDirIt = std::find_if(
        rootDirIt->subdirectories.begin(),
        rootDirIt->subdirectories.end(),
        [](const U8Archive::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != rootDirIt->subdirectories.end()) {
        return SessionManager::OpenError_LayoutArchive;
    }

    rootDirIt->SortAlphabetically();

    U8Archive::File* __tplSearch = U8Archive::findFile("cellanim.tpl", *rootDirIt);
    if (!__tplSearch) {
        return SessionManager::OpenError_FailFindTPL;
    }

    TPL::TPLObject tplObject (__tplSearch->data.data(), __tplSearch->data.size());
    if (!tplObject.isInitialized()) {
        return SessionManager::OpenError_FailOpenTPL;
    }

    std::vector<const U8Archive::File*> brcadFiles;
    for (const auto& file : rootDirIt->files) {
        if (
            file.name.size() >= 6 &&
            file.name.substr(file.name.size() - STR_LIT_LEN(".brcad")) == ".brcad"
        )
            brcadFiles.push_back(&file);
    }

    if (brcadFiles.empty()) {
        return SessionManager::OpenError_NoBXCADsFound;
    }

    // Sort cellanim files alphabetically for consistency when exporting.
    std::sort(
        brcadFiles.begin(), brcadFiles.end(),
        [](const U8Archive::File*& a, const U8Archive::File*& b) {
            return a->name < b->name;
        }
    );

    session.cellanims.resize(brcadFiles.size());

    session.sheets->getVector().reserve(tplObject.textures.size());

    // Cellanims
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        auto& cellanim = session.cellanims[i];
        const auto* file = brcadFiles[i];

        cellanim.name = file->name.substr(0, file->name.size() - STR_LIT_LEN(".brcad"));
        cellanim.object =
            std::make_shared<CellAnim::CellAnimObject>(
                file->data.data(), file->data.size()
            );

        if (
            !cellanim.object->isInitialized() ||
            cellanim.object->getType() != CellAnim::CELLANIM_TYPE_RVL
        )
            return SessionManager::OpenError_FailOpenBXCAD;
    }

    // Headers (animation names)
    for (unsigned i = 0; i < brcadFiles.size(); i++) {
        const U8Archive::File* brcadFile = brcadFiles[i];
        const U8Archive::File* headerFile { nullptr };

        unsigned cellanimNameLen = brcadFile->name.size() - STR_LIT_LEN(".brcad");

        char targetHeaderName[128];
        snprintf(
            targetHeaderName, sizeof(targetHeaderName),
            "rcad_%.*s_labels.h",
            static_cast<int>(cellanimNameLen),
            brcadFile->name.c_str()
        );

        // Find header file
        for (const auto& file : rootDirIt->files) {
            if (strcmp(file.name.c_str(), targetHeaderName) == 0) {
                headerFile = &file;
                break;
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

                auto& animations = session.cellanims[i].object->animations;
                if (value < animations.size()) {
                    auto& animation = session.cellanims[i].object->animations.at(value);

                    // +1 because of the trailing underscore.
                    animation.name = key.substr(cellanimNameLen + 1);
                }
            }
        }
    }

    // Sheets
    // Note: this is wrapped in a MainThreadTask since we need to get access to the
    //       GL context to create GPU textures. This can be outside of a MainThreadTask
    //       but then it would use multiple MainThreadTasks instead of just one.
    MainThreadTaskManager::getInstance().QueueTask([&tplObject, &session]() {
        for (unsigned i = 0; i < tplObject.textures.size(); i++) {
            auto& texture = tplObject.textures[i];

            std::shared_ptr<Texture> sheet = std::make_shared<Texture>(
                texture.width, texture.height,
                texture.createGPUTexture()
            );
            sheet->setTPLOutputFormat(texture.format);

            session.sheets->addTexture(std::move(sheet));
        }
    }).get();

    // Editor data
    {
        U8Archive::File* tedSearch = U8Archive::findFile(TED_ARC_FILENAME, *rootDirIt);
        if (tedSearch)
            TedApply(tedSearch->data.data(), session);
        else {
            U8Archive::File* datSearch = U8Archive::findFile(TED_ARC_FILENAME_OLD, *rootDirIt);
            if (datSearch)
                TedApply(datSearch->data.data(), session);
        }
    }

    return SessionManager::Error_None;
}

static SessionManager::Error InitCtrSession(
    Session& session, SARC::SARCObject& archive
) {
    Logging::info << "[InitCtrSession] <> Stage: find lyt dir" << std::endl;

    // Every layout archive has the directory "blyt". If this directory exists
    // we should throw an error
    auto blytDirIt = std::find_if(
        archive.structure.subdirectories.begin(),
        archive.structure.subdirectories.end(),
        [](const SARC::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != archive.structure.subdirectories.end()) {
        return SessionManager::OpenError_LayoutArchive;
    }

    Logging::info << "[InitCtrSession] <> Stage: find arc dir" << std::endl;

    auto rootDirIt = std::find_if(
        archive.structure.subdirectories.begin(),
        archive.structure.subdirectories.end(),
        [](const SARC::Directory& dir) { return dir.name == "arc"; }
    );

    if (rootDirIt == archive.structure.subdirectories.end())
        return SessionManager::OpenError_RootDirNotFound;

    Logging::info << "[InitCtrSession] <> Stage: collect bccad" << std::endl;

    std::vector<const SARC::File*> bccadFiles;
    for (const auto& file : rootDirIt->files) {
        if (
            file.name.size() >= STR_LIT_LEN(".bccad") &&
            file.name.substr(file.name.size() - STR_LIT_LEN(".bccad")) == ".bccad"
        )
            bccadFiles.push_back(&file);
    }

    if (bccadFiles.empty())
        return SessionManager::OpenError_NoBXCADsFound;

    Logging::info << "[InitCtrSession] <> Stage: collect & match ctpk" << std::endl;

    std::vector<const SARC::File*> ctpkFiles;

    for (const auto* bccadFile : bccadFiles) {
        std::string baseName = bccadFile->name.substr(0, bccadFile->name.find_last_of('.'));

        bool foundMatch = false;
        const SARC::File* bestMatch = nullptr;
        size_t bestMatchLength = 0;

        for (const auto& file : rootDirIt->files) {
            if (file.name.size() >= STR_LIT_LEN(".ctpk") &&
                file.name.substr(file.name.size() - STR_LIT_LEN(".ctpk")) == ".ctpk") {

                std::string ctpkBaseName = file.name.substr(0, file.name.find_last_of('.'));

                // HACK: because of some edge-cases where the developers didn't exactly mirror the
                // BCCAD and CTPK filenames, we have to do a substring search -_-

                if (baseName.find(ctpkBaseName) != std::string::npos) {
                    // Prioritize longest match.
                    if (ctpkBaseName.size() > bestMatchLength) {
                        bestMatch = &file;
                        bestMatchLength = ctpkBaseName.size();
                    }
                }
            }
        }

        if (bestMatch) {
            ctpkFiles.push_back(bestMatch);
            foundMatch = true;
        }

        if (!foundMatch)
            return SessionManager::OpenError_MissingCTPK;
    }

    Logging::info << "[InitCtrSession] <> Stage: start init cellanim" << std::endl;

    session.cellanims.resize(bccadFiles.size());
    session.sheets->getVector().reserve(ctpkFiles.size());

    // Cellanims
    for (unsigned i = 0; i < bccadFiles.size(); i++) {
        Logging::info << "[InitCtrSession] <> Stage: i=" << i << std::endl;

        auto& cellanim = session.cellanims[i];
        const auto* file = bccadFiles[i];

        cellanim.name = file->name.substr(0, file->name.size() - STR_LIT_LEN(".bccad"));

        Logging::info << "[InitCtrSession] <> Stage: init cellanim object" << std::endl;
        cellanim.object = std::make_shared<CellAnim::CellAnimObject>(
            file->data.data(), file->data.size()
        );

        if (
            !cellanim.object->isInitialized() ||
            cellanim.object->getType() != CellAnim::CELLANIM_TYPE_CTR
        )
            return SessionManager::OpenError_FailOpenBXCAD;

        cellanim.object->sheetIndex = i;
    }

    // Sheets
    // Note: this is wrapped in a MainThreadTask since we need to get access to the
    //       GL context to create GPU textures. This can be outside of a MainThreadTask
    //       but then it would use multiple MainThreadTasks instead of just one.
    SessionManager::Error sheetsError { SessionManager::Error_None };

    Logging::info << "[InitCtrSession] <> Stage: queue & wait for texture init task" << std::endl;

    MainThreadTaskManager::getInstance().QueueTask([&sheetsError, &ctpkFiles, &session]() {
        for (unsigned i = 0; i < ctpkFiles.size(); i++) {
            Logging::info << "[texture init task] <> Stage: i=" << i << std::endl;

            const auto* file = ctpkFiles[i];

            Logging::info << "[texture init task] <> Stage: init CTPK object" << std::endl;

            CTPK::CTPKObject ctpkObject = CTPK::CTPKObject(
                file->data.data(), file->data.size()
            );
            if (!ctpkObject.isInitialized()) {
                sheetsError = SessionManager::OpenError_FailOpenCTPK;
                return;
            }

            if (ctpkObject.textures.empty()) {
                sheetsError = SessionManager::OpenError_NoCTPKTextures;
                return;
            }

            Logging::info << "[texture init task] <> Stage: rotate tex" << std::endl;

            auto& texture = ctpkObject.textures[0];
            texture.rotateCCW();

            Logging::info << "[texture init task] <> Stage: make GPU texture" << std::endl;

            std::shared_ptr<Texture> sheet = std::make_shared<Texture>(
                texture.width, texture.height,
                texture.createGPUTexture()
            );
            sheet->setCTPKOutputFormat(texture.format);
            sheet->setOutputMipCount(texture.mipCount);
            sheet->setName(file->name.substr(0, file->name.size() - STR_LIT_LEN(".ctpk")));

            Logging::info << "[texture init task] <> Stage: move into sheets" << std::endl;

            session.sheets->addTexture(std::move(sheet));
        }
    }).get();

    if (sheetsError != SessionManager::Error_None)
        return sheetsError;

    // Editor data
    SARC::File* tedSearch = SARC::findFile(TED_ARC_FILENAME, *rootDirIt);
    if (tedSearch)
        TedApply(tedSearch->data.data(), session);

    return SessionManager::Error_None;
}

int SessionManager::CreateSession(std::string_view filePath) {
    if (!Files::doesFileExist(filePath)) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_FileDoesNotExist;

        return -1;
    }

    Logging::info <<
        "[SessionManager::CreateSession] Creating session from path \"" << filePath << "\".." << std::endl;

    Logging::info << "[SessionManager::CreateSession] <> Stage: read file data" << std::endl;

    std::vector<unsigned char> data;
    {
        std::ifstream file(filePath.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            Logging::err << "[SessionManager::CreateSession] Error opening file at path: " << filePath << std::endl;

            std::lock_guard<std::mutex> lock(this->mtx);
            this->currentError = OpenError_FailOpenFile;

            return -1;
        }

        data.resize(file.tellg());
        file.seekg(0, std::ios::beg);

        file.read(reinterpret_cast<char*>(data.data()), data.size());

        file.close();
    }

    Logging::info << "[SessionManager::CreateSession] <> Stage: check data valid" << std::endl;

    CellAnim::CellAnimType type { CellAnim::CELLANIM_TYPE_INVALID };

    // We check for Yaz0 first since it has a magic value.
    if (Yaz0::checkDataValid(data.data(), data.size())) {
        type = CellAnim::CELLANIM_TYPE_RVL;

        const auto decompressedData = Yaz0::decompress(data.data(), data.size());
        if (!decompressedData.has_value()) {
            Logging::err << "[SessionManager::CreateSession] Failed to decompress Yaz0 data!" << std::endl;

            std::lock_guard<std::mutex> lock(this->mtx);
            this->currentError = OpenError_FailOpenArchive;

            return -1;
        }

        data = std::move(*decompressedData);
    }
    else if (NZlib::checkDataValid(data.data(), data.size())) {
        type = CellAnim::CELLANIM_TYPE_CTR;

        Logging::info << "[SessionManager::CreateSession] <> Stage: NZlib decomp" << std::endl;

        const auto decompressedData = NZlib::decompress(data.data(), data.size());
        if (!decompressedData.has_value()) {
            Logging::err << "[SessionManager::CreateSession] Failed to decompress NZlib data!" << std::endl;

            std::lock_guard<std::mutex> lock(this->mtx);
            this->currentError = OpenError_FailOpenArchive;

            return -1;
        }

        Logging::info << "[SessionManager::CreateSession] <> Stage: move decomped data" << std::endl;

        data = std::move(*decompressedData);
    }
    else {
        Logging::err << "[SessionManager::CreateSession] Data doesn't match Yaz0 or NZlib.." << std::endl;

        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = OpenError_FailOpenArchive;

        return -1;
    }

    Error initError { Error_None };

    Session newSession;

    switch (type) {
    case CellAnim::CELLANIM_TYPE_RVL: {
        U8Archive::U8ArchiveObject archive = U8Archive::U8ArchiveObject(
            data.data(), data.size()
        );

        if (!archive.isInitialized()) {
            initError = OpenError_FailOpenArchive;
            break;
        }

        initError = InitRvlSession(newSession, archive);
    } break;
    case CellAnim::CELLANIM_TYPE_CTR: {
        Logging::info << "[SessionManager::CreateSession] <> Stage: construct SARC" << std::endl;

        SARC::SARCObject archive = SARC::SARCObject(data.data(), data.size());

        if (!archive.isInitialized()) {
            const uint32_t observedMagic = *reinterpret_cast<const uint32_t*>(data.data());
            switch (observedMagic) {
            case IDENTIFIER_TO_U32('C','G','F','X'): // BCRES
                initError = OpenError_BCRES;
                break;
            case IDENTIFIER_TO_U32('S','P','B','D'): // Effect
                initError = OpenError_EffectResource;
                break;

            default:
                initError = OpenError_FailOpenArchive;
                break;
            }

            break;
        }

        Logging::info << "[SessionManager::CreateSession] <> Stage: start CTR init" << std::endl;

        initError = InitCtrSession(newSession, archive);
    } break;

    default:
        initError = OpenError_FailOpenArchive;
        break;
    }

    if (initError != Error_None) {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->currentError = initError;

        return -1;
    }

    newSession.type = type;

    newSession.resourcePath = filePath;
    newSession.setCurrentCellAnimIndex(0);

    std::lock_guard<std::mutex> lock(this->mtx);

    this->sessions.push_back(std::move(newSession));
    const int sessionIndex = this->sessions.size() - 1;

    this->currentError = Error_None;

    Logging::info <<
        "[SessionManager::CreateSession] Created session no. " << sessionIndex+1 << '.' << std::endl;

    return static_cast<int>(this->sessions.size() - 1);
}

static SessionManager::Error SerializeRvlSession(
    const Session& session, std::vector<unsigned char>& output
) {
    U8Archive::U8ArchiveObject archive;

    archive.structure.AddDirectory(U8Archive::Directory("."));
    auto& directory = archive.structure.subdirectories.back();

    // BRCAD files
    for (unsigned i = 0; i < session.cellanims.size(); i++) {
        U8Archive::File file(session.cellanims[i].name + ".brcad");

        Logging::info <<
            "[SerializeRvlSession] Serializing cellanim \"" << session.cellanims[i].name << "\".." << std::endl;

        file.data = session.cellanims[i].object->Serialize();

        directory.AddFile(std::move(file));
    }

    // Header files
    for (unsigned i = 0; i < session.cellanims.size(); i++) {
        const std::string& cellanimName = session.cellanims[i].name;

        U8Archive::File file(
            "rcad_" + cellanimName + "_labels.h"
        );

        Logging::info <<
            "[SerializeRvlSession] Writing label header for cellanim \"" << cellanimName << "\".." << std::endl;

        std::ostringstream stream;
        for (unsigned j = 0; j < session.cellanims[i].object->animations.size(); j++) {
            const auto& animation = session.cellanims[i].object->animations[j];
            if (animation.name.empty())
                continue;

            // Note: Bread requires a comment at the end, so we write it for reverse-compatibillity.
            stream << "#define " << cellanimName << '_' << animation.name << '\t' << std::to_string(j) << "\t// (null)\n";
        }

        const std::string str = stream.str();
        file.data.insert(file.data.end(), str.begin(), str.end());

        directory.AddFile(std::move(file));
    }

    // TPL file
    {
        U8Archive::File file("cellanim.tpl");

        TPL::TPLObject tplObject;

        SessionManager::Error error { SessionManager::Error_None };
        MainThreadTaskManager::getInstance().QueueTask([&session, &tplObject, &error]() {
            tplObject.textures.resize(session.sheets->getTextureCount());
            for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
                auto tplTexture = session.sheets->getTextureByIndex(i)->TPLTexture();
                if (!tplTexture.has_value()) {
                    error = SessionManager::OutError_FailTextureExport;
                    return;
                }

                tplObject.textures[i] = std::move(*tplTexture);
            }
        }).get();

        if (error != SessionManager::Error_None)
            return error;

        Logging::info << "[SerializeRvlSession] Serializing textures.." << std::endl;

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

    Logging::info << "[SerializeRvlSession] Serializing archive.." << std::endl;

    directory.SortAlphabetically();

    auto archiveBinary = archive.Serialize();

    Logging::info << "[SerializeRvlSession] Compressing archive.." << std::endl;

    auto compressedArchive = Yaz0::compress(
        archiveBinary.data(), archiveBinary.size(),
        ConfigManager::getInstance().getConfig().compressionLevel
    );

    if (!compressedArchive.has_value())
        return SessionManager::OutError_ZlibError;

    output = std::move(*compressedArchive);
    compressedArchive.reset();

    return SessionManager::Error_None;
}

static SessionManager::Error SerializeCtrSession(
    const Session& session, std::vector<unsigned char>& output
) {
    SARC::SARCObject archive;

    archive.structure.AddDirectory(SARC::Directory("arc"));
    auto& directory = archive.structure.subdirectories.back();

    // BCCAD files
    for (unsigned i = 0; i < session.cellanims.size(); i++) {
        const auto& cellanim = session.cellanims[i];

        SARC::File file(cellanim.name + ".bccad");

        Logging::info <<
            "[SerializeCtrSession] Serializing cellanim \"" << cellanim.name << "\".." << std::endl;

        file.data = cellanim.object->Serialize();

        directory.AddFile(std::move(file));
    }

    // CTPK files
    std::vector<CTPK::CTPKTexture> ctpkTextures;

    SessionManager::Error getTexturesError { SessionManager::Error_None };
    MainThreadTaskManager::getInstance().QueueTask([&session, &ctpkTextures, &getTexturesError]() {
        ctpkTextures.resize(session.sheets->getTextureCount());
        for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
            auto ctpkTexture = session.sheets->getTextureByIndex(i)->CTPKTexture();
            if (!ctpkTexture.has_value()) {
                getTexturesError = SessionManager::OutError_FailTextureExport;
                return;
            }

            ctpkTextures[i] = std::move(*ctpkTexture);
        }
    }).get();

    if (getTexturesError != SessionManager::Error_None)
        return getTexturesError;

    for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
        const auto& texture = session.sheets->getTextureByIndex(i);
        auto& ctpkTex = ctpkTextures[i];

        SARC::File file(texture->getName() + ".ctpk");

        ctpkTex.rotateCW();

        ctpkTex.sourcePath = "data/" + texture->getName() + "_rot.tga";

        CTPK::CTPKObject ctpkObject;
        ctpkObject.textures.assign(1, std::move(ctpkTex));

        Logging::info << "[SerializeCtrSession] Serializing texture \"" << texture->getName() << "\".." << std::endl;

        file.data = ctpkObject.Serialize();

        directory.AddFile(std::move(file));
    }

    ctpkTextures.clear();

    // TED file
    {
        SARC::File file(TED_ARC_FILENAME);

        TedWriteState* state = TedCreateWriteState(session);
            file.data.resize(TedPrepareWrite(state));
            TedWrite(state, file.data.data());
        TedDestroyWriteState(state);

        directory.AddFile(std::move(file));
    }

    Logging::info << "[SerializeCtrSession] Serializing archive.." << std::endl;

    auto archiveBinary = archive.Serialize();

    Logging::info << "[SerializeCtrSession] Compressing archive.." << std::endl;

    auto compressedArchive = NZlib::compress(
        archiveBinary.data(), archiveBinary.size(),
        ConfigManager::getInstance().getConfig().compressionLevel
    );

    if (!compressedArchive.has_value())
        return SessionManager::OutError_ZlibError;

    output = std::move(*compressedArchive);
    compressedArchive.reset();

    return SessionManager::Error_None;
}

bool SessionManager::ExportSession(unsigned sessionIndex, std::string_view dstFilePath) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (sessionIndex >= this->sessions.size())
        return false;

    auto& session = this->sessions[sessionIndex];

    if (dstFilePath.empty())
        dstFilePath = session.resourcePath;

    Logging::info <<
        "[SessionManager::ExportSession] Exporting session no. " << sessionIndex+1 <<
        " to path \"" << dstFilePath << "\".." << std::endl;

    Error error;
    std::vector<unsigned char> result;

    switch (session.type) {
    case CellAnim::CELLANIM_TYPE_RVL:
        error = SerializeRvlSession(session, result);
        break;
    case CellAnim::CELLANIM_TYPE_CTR:
        error = SerializeCtrSession(session, result);
        break;

    default:
        Logging::err << "[SessionManager::ExportSession] Session type is invalid (" << static_cast<int>(session.type) << ")!" << std::endl;
        error = Error_Unknown;
        break;
    }

    if (error != Error_None) {
        this->currentError = error;
        return false;
    }

    const ConfigManager& configManager = ConfigManager::getInstance();

    if (
        Files::doesFileExist(dstFilePath) && 
        configManager.getConfig().backupBehaviour != BackupBehaviour_None
    ) {
        std::string backupPath = std::string(dstFilePath) + ".bak";

        Logging::info << "[SessionManager::ExportSession] Backing up file at \"" 
                    << dstFilePath << "\" to \"" << backupPath << "\".." << std::endl;

        bool success = Files::copyFile(
            dstFilePath, backupPath,
            configManager.getConfig().backupBehaviour == BackupBehaviour_SaveOverwrite
        );

        if (!success) {
            Logging::err << "[SessionManager::ExportSession] Failed to save backup of file! Aborting.." << std::endl;
            this->currentError = OutError_FailBackupFile;
            return false;
        }
    }

    std::ofstream file(dstFilePath, std::ios::binary);
    if (file.is_open()) {
        file.write(
            reinterpret_cast<const char*>(result.data()),
            result.size()
        );

        file.close();
    }
    else {
        this->currentError = OutError_FailOpenFile;
        return false;
    }

    session.modified = false;

    Logging::info <<
            "[SessionManager::ExportSession] Finished exporting session no. " << sessionIndex+1 << '.' << std::endl;

    return true;
}

void SessionManager::RemoveSession(unsigned sessionIndex) {
    std::lock_guard<std::mutex> lock(this->mtx);

    if (sessionIndex >= this->sessions.size())
        return;

    Logging::info << "[SessionManager::RemoveSession] Removing session no. " << sessionIndex+1 << ".." << std::endl;

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

    unsigned sessionCount = this->sessions.size();

    this->sessions.clear();
    this->currentSessionIndex = -1;

    if (sessionCount != 0)
        Logging::info << "[SessionManager::RemoveAllSessions] Removed all sessions (" << sessionCount << ")." << std::endl;
}
