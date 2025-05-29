#include "SessionManager.hpp"

#include <cstdio>
#include <cstring>

#include <sstream>
#include <fstream>

#include "Logging.hpp"

#include <algorithm>

#include "cellanim/CellAnim.hpp"

#include "archive/Archive.hpp"

#include "archive/DARCH.hpp"
#include "archive/SARC.hpp"

#include "compression/Yaz0.hpp"
#include "compression/NZlib.hpp"

#include "texture/TPL.hpp"
#include "texture/CTPK.hpp"

#include "texture/TextureEx.hpp"

#include "EditorDataPackage.hpp"

#include "ConfigManager.hpp"
#include "PlayerManager.hpp"
#include "MainThreadTaskManager.hpp"

#include "Files.hpp"

#include "common.hpp"

void SessionManager::setCurrentSessionIndex(int sessionIndex) {
    std::lock_guard<std::mutex> lock(mMtx);

    if (sessionIndex >= mSessions.size())
        return;

    mCurrentSessionIndex = sessionIndex;

    PlayerManager::getInstance().correctState();

    Logging::info << "[SessionManager::setCurrentSessionIndex] Selected session no. " << sessionIndex+1 << '.' << std::endl;
}

static SessionManager::Error InitRvlSession(
    Session& session, const Archive::DARCHObject& archive
) {
    auto rootDirIt = std::find_if(
        archive.getStructure().subdirectories.begin(),
        archive.getStructure().subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "."; }
    );

    if (rootDirIt == archive.getStructure().subdirectories.end()) {
        return SessionManager::OpenError_RootDirNotFound;
    }

    // Every layout archive has the directory "./blyt". If this directory exists
    // we should throw an error
    auto blytDirIt = std::find_if(
        rootDirIt->subdirectories.begin(),
        rootDirIt->subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != rootDirIt->subdirectories.end()) {
        return SessionManager::OpenError_LayoutArchive;
    }

    const Archive::File* __tplSearch = Archive::findFile("cellanim.tpl", *rootDirIt);
    if (!__tplSearch) {
        return SessionManager::OpenError_FailFindTPL;
    }

    TPL::TPLObject tplObject (__tplSearch->data.data(), __tplSearch->data.size());
    if (!tplObject.isInitialized()) {
        return SessionManager::OpenError_FailOpenTPL;
    }

    std::vector<const Archive::File*> brcadFiles;
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
        [](const Archive::File*& a, const Archive::File*& b) {
            return a->name < b->name;
        }
    );

    session.cellanims.resize(brcadFiles.size());

    session.sheets->getVector().reserve(tplObject.mTextures.size());

    // Cellanims
    for (size_t i = 0; i < brcadFiles.size(); i++) {
        Session::CellAnimGroup& cellanim = session.cellanims[i];
        const Archive::File* file = brcadFiles[i];

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
    for (size_t i = 0; i < brcadFiles.size(); i++) {
        const Archive::File* brcadFile = brcadFiles[i];
        const Archive::File* headerFile { nullptr };

        int cellanimNameLen = brcadFile->name.size() - STR_LIT_LEN(".brcad");

        char targetHeaderName[128];
        snprintf(
            targetHeaderName, sizeof(targetHeaderName),
            "rcad_%.*s_labels.h",
            cellanimNameLen,
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

                auto& animations = session.cellanims[i].object->getAnimations();
                if (value < animations.size()) {
                    auto& animation = session.cellanims[i].object->getAnimation(value);

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
        for (auto& texture : tplObject.mTextures) {
            std::shared_ptr<TextureEx> sheet = std::make_shared<TextureEx>(
                texture.width, texture.height,
                texture.createGPUTexture()
            );
            sheet->setTPLOutputFormat(texture.format);

            session.sheets->addTexture(std::move(sheet));
        }
    }).get();

    // Editor data
    {
        const Archive::File* tedSearch = Archive::findFile(TED_ARC_FILENAME, *rootDirIt);
        if (tedSearch)
            TedApply(tedSearch->data.data(), session);
        else {
            const Archive::File* datSearch = Archive::findFile(TED_ARC_FILENAME_OLD, *rootDirIt);
            if (datSearch)
                TedApply(datSearch->data.data(), session);
        }
    }

    return SessionManager::Error_None;
}

static SessionManager::Error InitCtrSession(
    Session& session, const Archive::SARCObject& archive
) {
    // Every layout archive has the directory "blyt". If this directory exists
    // we should throw an error
    auto blytDirIt = std::find_if(
        archive.getStructure().subdirectories.begin(),
        archive.getStructure().subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != archive.getStructure().subdirectories.end()) {
        return SessionManager::OpenError_LayoutArchive;
    }

    auto rootDirIt = std::find_if(
        archive.getStructure().subdirectories.begin(),
        archive.getStructure().subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "arc"; }
    );

    if (rootDirIt == archive.getStructure().subdirectories.end())
        return SessionManager::OpenError_RootDirNotFound;

    std::vector<const Archive::File*> bccadFiles;
    for (const auto& file : rootDirIt->files) {
        if (
            file.name.size() >= STR_LIT_LEN(".bccad") &&
            file.name.substr(file.name.size() - STR_LIT_LEN(".bccad")) == ".bccad"
        )
            bccadFiles.push_back(&file);
    }

    if (bccadFiles.empty())
        return SessionManager::OpenError_NoBXCADsFound;

    std::vector<const Archive::File*> ctpkFiles;

    for (const auto* bccadFile : bccadFiles) {
        std::string baseName = bccadFile->name.substr(0, bccadFile->name.find_last_of('.'));

        bool foundMatch = false;
        const Archive::File* bestMatch = nullptr;
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

    session.cellanims.resize(bccadFiles.size());
    session.sheets->getVector().reserve(ctpkFiles.size());

    // Cellanims
    for (size_t i = 0; i < bccadFiles.size(); i++) {
        auto& cellanim = session.cellanims[i];
        const auto* file = bccadFiles[i];

        cellanim.name = file->name.substr(0, file->name.size() - STR_LIT_LEN(".bccad"));
        cellanim.object = std::make_shared<CellAnim::CellAnimObject>(
            file->data.data(), file->data.size()
        );

        if (
            !cellanim.object->isInitialized() ||
            cellanim.object->getType() != CellAnim::CELLANIM_TYPE_CTR
        )
            return SessionManager::OpenError_FailOpenBXCAD;

        cellanim.object->setSheetIndex(i);
    }

    // Sheets
    // Note: this is wrapped in a MainThreadTask since we need to get access to the
    //       GL context to create GPU textures. This can be outside of a MainThreadTask
    //       but then it would use multiple MainThreadTasks instead of just one.
    SessionManager::Error sheetsError { SessionManager::Error_None };

    MainThreadTaskManager::getInstance().QueueTask([&sheetsError, &ctpkFiles, &session]() {
        for (size_t i = 0; i < ctpkFiles.size(); i++) {
            const auto* file = ctpkFiles[i];

            CTPK::CTPKObject ctpkObject = CTPK::CTPKObject(
                file->data.data(), file->data.size()
            );
            if (!ctpkObject.isInitialized()) {
                sheetsError = SessionManager::OpenError_FailOpenCTPK;
                return;
            }

            if (ctpkObject.mTextures.empty()) {
                sheetsError = SessionManager::OpenError_NoCTPKTextures;
                return;
            }

            auto& texture = ctpkObject.mTextures[0];
            texture.rotateCCW();

            std::shared_ptr<TextureEx> sheet = std::make_shared<TextureEx>(
                texture.width, texture.height,
                texture.createGPUTexture()
            );
            sheet->setOutputMipCount(texture.mipCount);
            sheet->setCTPKOutputFormat(texture.targetFormat);
            sheet->setOutputSrcTimestamp(texture.sourceTimestamp);
            sheet->setOutputSrcPath(texture.sourcePath);
            sheet->setName(file->name.substr(0, file->name.size() - STR_LIT_LEN(".ctpk")));

            session.sheets->addTexture(std::move(sheet));
        }
    }).get();

    if (sheetsError != SessionManager::Error_None)
        return sheetsError;

    // Editor data
    const Archive::File* tedSearch = Archive::findFile(TED_ARC_FILENAME, *rootDirIt);
    if (tedSearch)
        TedApply(tedSearch->data.data(), session);

    return SessionManager::Error_None;
}

int SessionManager::CreateSession(std::string_view filePath) {
    if (!Files::doesFileExist(filePath)) {
        std::lock_guard<std::mutex> lock(mMtx);
        mCurrentError = OpenError_FileDoesNotExist;

        return -1;
    }

    Logging::info <<
        "[SessionManager::CreateSession] Creating session from path \"" << filePath << "\".." << std::endl;

    std::vector<unsigned char> data;
    {
        std::ifstream file(filePath.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            Logging::err << "[SessionManager::CreateSession] Error opening file at path: " << filePath << std::endl;

            std::lock_guard<std::mutex> lock(mMtx);
            mCurrentError = OpenError_FailOpenFile;

            return -1;
        }

        data.resize(file.tellg());
        file.seekg(0, std::ios::beg);

        file.read(reinterpret_cast<char*>(data.data()), data.size());

        file.close();
    }

    CellAnim::CellAnimType type { CellAnim::CELLANIM_TYPE_INVALID };

    // We check for Yaz0 first since it has a magic value.
    if (Yaz0::checkDataValid(data.data(), data.size())) {
        type = CellAnim::CELLANIM_TYPE_RVL;

        const auto decompressedData = Yaz0::decompress(data.data(), data.size());
        if (!decompressedData.has_value()) {
            Logging::err << "[SessionManager::CreateSession] Failed to decompress Yaz0 data!" << std::endl;

            std::lock_guard<std::mutex> lock(mMtx);
            mCurrentError = OpenError_FailOpenArchive;

            return -1;
        }

        data = std::move(*decompressedData);
    }
    else if (NZlib::checkDataValid(data.data(), data.size())) {
        type = CellAnim::CELLANIM_TYPE_CTR;

        const auto decompressedData = NZlib::decompress(data.data(), data.size());
        if (!decompressedData.has_value()) {
            std::lock_guard<std::mutex> lock(mMtx);
            mCurrentError = OpenError_FailOpenArchive;

            return -1;
        }

        data = std::move(*decompressedData);
    }
    else {
        std::lock_guard<std::mutex> lock(mMtx);
        mCurrentError = OpenError_FailOpenArchive;

        return -1;
    }

    Error initError { Error_None };

    Session newSession;

    switch (type) {
    case CellAnim::CELLANIM_TYPE_RVL: {
        Archive::DARCHObject archive = Archive::DARCHObject(
            data.data(), data.size()
        );

        if (!archive.isInitialized()) {
            initError = OpenError_FailOpenArchive;
            break;
        }

        initError = InitRvlSession(newSession, archive);
    } break;
    case CellAnim::CELLANIM_TYPE_CTR: {
        Archive::SARCObject archive = Archive::SARCObject(data.data(), data.size());

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

        initError = InitCtrSession(newSession, archive);
    } break;

    default:
        initError = OpenError_FailOpenArchive;
        break;
    }

    if (initError != Error_None) {
        std::lock_guard<std::mutex> lock(mMtx);
        mCurrentError = initError;

        return -1;
    }

    newSession.type = type;

    newSession.resourcePath = filePath;
    newSession.setCurrentCellAnimIndex(0);

    std::lock_guard<std::mutex> lock(mMtx);

    mSessions.push_back(std::move(newSession));
    const int sessionIndex = mSessions.size() - 1;

    mCurrentError = Error_None;

    Logging::info <<
        "[SessionManager::CreateSession] Created session no. " << sessionIndex+1 << '.' << std::endl;

    return static_cast<int>(mSessions.size() - 1);
}

static SessionManager::Error SerializeRvlSession(
    const Session& session, std::vector<unsigned char>& output
) {
    Archive::DARCHObject archive;

    archive.getStructure().AddDirectory(Archive::Directory("."));
    auto& directory = archive.getStructure().subdirectories.back();

    // BRCAD files
    for (size_t i = 0; i < session.cellanims.size(); i++) {
        const auto& cellanim = session.cellanims[i];

        Archive::File file(cellanim.name + ".brcad");

        Logging::info <<
            "[SerializeRvlSession] Serializing cellanim \"" << cellanim.name << "\".." << std::endl;

        const auto& sheet = session.sheets->getTextureByIndex(session.cellanims[i].object->getSheetIndex());

        // Make sure usePalette is synced.
        session.cellanims[i].object->setUsePalette(TPL::getImageFormatPaletted(sheet->getTPLOutputFormat()));

        file.data = session.cellanims[i].object->Serialize();

        directory.AddFile(std::move(file));
    }

    // Header files
    for (size_t i = 0; i < session.cellanims.size(); i++) {
        const std::string& cellanimName = session.cellanims[i].name;

        Archive::File file(
            "rcad_" + cellanimName + "_labels.h"
        );

        Logging::info <<
            "[SerializeRvlSession] Writing label header for cellanim \"" << cellanimName << "\".." << std::endl;

        std::ostringstream stream;
        for (size_t j = 0; j < session.cellanims[i].object->getAnimations().size(); j++) {
            const auto& animation = session.cellanims[i].object->getAnimation(j);
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
        Archive::File file("cellanim.tpl");

        TPL::TPLObject tplObject;

        SessionManager::Error error { SessionManager::Error_None };
        MainThreadTaskManager::getInstance().QueueTask([&session, &tplObject, &error]() {
            tplObject.mTextures.resize(session.sheets->getTextureCount());
            for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
                const auto& sheet = session.sheets->getTextureByIndex(i);

                auto tplTexture = sheet->TPLTexture();
                if (!tplTexture.has_value()) {
                    error = SessionManager::OutError_FailTextureExport;
                    return;
                }

                tplObject.mTextures[i] = std::move(*tplTexture);
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
        Archive::File file(TED_ARC_FILENAME);

        TedWriteState* state = TedCreateWriteState(session);
            file.data.resize(TedPrepareWrite(state));
            TedWrite(state, file.data.data());
        TedDestroyWriteState(state);

        directory.AddFile(std::move(file));
    }

    Logging::info << "[SerializeRvlSession] Serializing archive.." << std::endl;

    directory.SortAlphabetic();

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
    Archive::SARCObject archive;

    archive.getStructure().AddDirectory(Archive::Directory("arc"));
    auto& directory = archive.getStructure().subdirectories.back();

    // BCCAD files
    for (size_t i = 0; i < session.cellanims.size(); i++) {
        const auto& cellanim = session.cellanims[i];

        Archive::File file(cellanim.name + ".bccad");

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
            const auto& sheet = session.sheets->getTextureByIndex(i);

            auto ctpkTexture = sheet->CTPKTexture();
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

        Archive::File file(texture->getName() + ".ctpk");

        ctpkTex.rotateCW();

        ctpkTex.sourcePath = "data/" + texture->getName() + "_rot.tga";

        CTPK::CTPKObject ctpkObject;
        ctpkObject.mTextures.assign(1, std::move(ctpkTex));

        Logging::info << "[SerializeCtrSession] Serializing texture \"" << texture->getName() << "\".." << std::endl;

        file.data = ctpkObject.Serialize();

        directory.AddFile(std::move(file));
    }

    ctpkTextures.clear();

    // TED file
    {
        Archive::File file(TED_ARC_FILENAME);

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
    std::lock_guard<std::mutex> lock(mMtx);

    if (sessionIndex >= mSessions.size())
        return false;

    auto& session = mSessions[sessionIndex];

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
        mCurrentError = error;
        return false;
    }

    const ConfigManager& configManager = ConfigManager::getInstance();

    if (
        Files::doesFileExist(dstFilePath) && 
        configManager.getConfig().backupBehaviour != BackupBehaviour::None
    ) {
        std::string backupPath = std::string(dstFilePath) + ".bak";

        Logging::info << "[SessionManager::ExportSession] Backing up file at \"" 
                    << dstFilePath << "\" to \"" << backupPath << "\".." << std::endl;

        bool success = Files::copyFile(
            dstFilePath, backupPath,
            configManager.getConfig().backupBehaviour == BackupBehaviour::SaveOverwrite
        );

        if (!success) {
            Logging::err << "[SessionManager::ExportSession] Failed to save backup of file! Aborting.." << std::endl;
            mCurrentError = OutError_FailBackupFile;
            return false;
        }
    }

    std::ofstream file(dstFilePath.data(), std::ios::binary);
    if (file.is_open()) {
        file.write(
            reinterpret_cast<const char*>(result.data()),
            result.size()
        );

        file.close();
    }
    else {
        mCurrentError = OutError_FailOpenFile;
        return false;
    }

    session.modified = false;

    Logging::info <<
            "[SessionManager::ExportSession] Finished exporting session no. " << sessionIndex+1 << '.' << std::endl;

    return true;
}

void SessionManager::RemoveSession(unsigned sessionIndex) {
    std::lock_guard<std::mutex> lock(mMtx);

    if (sessionIndex >= mSessions.size())
        return;

    Logging::info << "[SessionManager::RemoveSession] Removing session no. " << sessionIndex+1 << ".." << std::endl;

    auto sessionIt = mSessions.begin() + sessionIndex;
    mSessions.erase(sessionIt);

    if (mSessions.empty())
        mCurrentSessionIndex = -1;
    else {
        mCurrentSessionIndex = std::min(
            mCurrentSessionIndex,
            static_cast<int>(mSessions.size()) - 1
        );

        PlayerManager::getInstance().correctState();
    }
}

void SessionManager::RemoveAllSessions() {
    std::lock_guard<std::mutex> lock(mMtx);

    unsigned sessionCount = mSessions.size();

    mSessions.clear();
    mCurrentSessionIndex = -1;

    if (sessionCount != 0)
        Logging::info << "[SessionManager::RemoveAllSessions] Removed all sessions (" << sessionCount << ")." << std::endl;
}
