#include "SessionManager.hpp"

#include <cstddef>

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

#include "manager/ConfigManager.hpp"
#include "manager/PlayerManager.hpp"
#include "manager/MainThreadTaskManager.hpp"
#include "manager/PromptPopupManager.hpp"

#include "util/FileUtil.hpp"

#include "util/ShiftJISUtil.hpp"

#include "Macro.hpp"

Session& SessionManager::getSession(size_t index) {
    if (index >= mSessions.size()) {
        throw std::out_of_range(
            "SessionManager::getSession: bad session index (" +
            std::to_string(index) + " >= " + std::to_string(mSessions.size()) + ")"
        );
    }
    return mSessions[index];
}
const Session& SessionManager::getSession(size_t index) const {
    if (index >= mSessions.size()) {
        throw std::out_of_range(
            "SessionManager::getSession: bad session index (" +
            std::to_string(index) + " >= " + std::to_string(mSessions.size()) + ")"
        );
    }
    return mSessions[index];
}

Session* SessionManager::getCurrentSession() {
    if (mCurrentSessionIndex < 0)
        return nullptr;
    return &mSessions[mCurrentSessionIndex];
}
const Session* SessionManager::getCurrentSession() const {
    if (mCurrentSessionIndex < 0)
        return nullptr;
    return &mSessions[mCurrentSessionIndex];
}

void SessionManager::setCurrentSessionIndex(ssize_t index) {
    std::lock_guard<std::mutex> lock(mMtx);

    if (index >= mSessions.size()) {
        throw std::out_of_range(
            "SessionManager::setCurrentSessionIndex: bad session index (" +
            std::to_string(index) + " >= " + std::to_string(mSessions.size()) + ")"
        );
    }

    if (index >= 0) {
        mCurrentSessionIndex = index;
        Logging::info << "[SessionManager::setCurrentSessionIndex] Selected session no. " << mCurrentSessionIndex + 1 << '.' << std::endl;
    }
    else {
        mCurrentSessionIndex = -1;
        Logging::info << "[SessionManager::setCurrentSessionIndex] Deselected current session." << std::endl;
    }

    PlayerManager::getInstance().correctState();
}

bool SessionManager::isCurrentSessionModified() const {
    const Session* currentSession = getCurrentSession();
    if (currentSession) {
        return currentSession->modified;
    }
    else {
        return false;
    }
}

void SessionManager::setCurrentSessionModified(bool modified) {
    Session* currentSession = getCurrentSession();
    if (currentSession) {
        currentSession->modified = modified;
    }
    else {
        Logging::warn <<
            "[SessionManager::setCurrentSessionModified] Attempted to set current "
            "session modified when current session does not exist.." << std::endl;
    }
}

constexpr std::string_view CREATE_SESSION_ERR_POPUP_TITLE = "An error occurred while opening the session..";
constexpr std::string_view EXPORT_SESSION_ERR_POPUP_TITLE = "An error occurred while exporting the session..";

static bool InitRvlSession(Session& session, const Archive::DARCHObject& archive) {
    auto rootDirIt = std::find_if(
        archive.getStructure().subdirectories.begin(),
        archive.getStructure().subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "."; }
    );

    if (rootDirIt == archive.getStructure().subdirectories.end()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The archive does not contain the root directory: are you sure this is a cellanim archive?"
        ));
        return false;
    }

    // Every layout archive has the directory "./blyt". If this directory exists
    // we should throw an error.
    auto blytDirIt = std::find_if(
        rootDirIt->subdirectories.begin(),
        rootDirIt->subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != rootDirIt->subdirectories.end()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The selected file is a layout archive; please choose a cellanim archive instead."
        ));
        return false;
    }

    const Archive::File* __tplSearch = Archive::findFile("cellanim.tpl", *rootDirIt);
    if (!__tplSearch) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The texture file (cellanim.tpl) was not found: are you sure this is a cellanim archive?"
        ));
        return false;
    }

    TPL::TPLObject tplObject (__tplSearch->data.data(), __tplSearch->data.size());
    if (!tplObject.isInitialized()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The texture file (cellanim.tpl) could not be deserialized; it might be corrupted."
        ));
        return false;
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
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The archive does not contain any cellanim data files (.brcad): are you\n"
            "sure this is a cellanim archive?"
        ));
        return false;
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

    // Cellanims.
    for (size_t i = 0; i < brcadFiles.size(); i++) {
        Session::CellAnimGroup& cellanim = session.cellanims[i];
        const Archive::File* file = brcadFiles[i];

        cellanim.object =
        std::make_shared<CellAnim::CellAnimObject>(
            file->data.data(), file->data.size()
        );
        cellanim.object->setName(file->name.substr(0, file->name.size() - STR_LIT_LEN(".brcad")));

        if (!cellanim.object->isInitialized()) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "A cellanim data file (.brcad) could not be deserialized; it might be corrupted."
            ));
            return false;
        }

        if (cellanim.object->getType() != CellAnim::CELLANIM_TYPE_RVL) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "A cellanim data file (.brcad) is in the wrong format (expected Wii, got 3DS)."
            ));
            return false;
        }
    }

    // Headers (animation names).
    for (size_t i = 0; i < brcadFiles.size(); i++) {
        const Archive::File* brcadFile = brcadFiles[i];
        const Archive::File* headerFile { nullptr };

        int cellanimNameLen = brcadFile->name.size() - STR_LIT_LEN(".brcad");

        // Find header file
        std::string targetHeaderName = "rcad_" + brcadFile->name.substr(0, cellanimNameLen) + "_labels.h";
        for (const auto& file : rootDirIt->files) {
            if (file.name == targetHeaderName) {
                headerFile = &file;
                break;
            }
        }

        if (!headerFile)
            continue;

        std::istringstream stringStream(ShiftJISUtil::convertToUTF8(
            reinterpret_cast<const char*>(headerFile->data.data()), headerFile->data.size()
        ));
        std::string line;

        while (std::getline(stringStream, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            if (line.compare(0, 7, "#define") == 0) {
                std::istringstream lineStream(line);
                std::string defineTag, key;
                unsigned value;

                lineStream >> defineTag >> key >> value;

                std::string comment;
                std::getline(lineStream, comment);

                size_t commentStart = comment.find_first_not_of(" \t//");
                if (commentStart != std::string::npos) {
                    comment = comment.substr(commentStart);
                }
                else {
                    comment.clear(); // No comment.
                }

                auto& animations = session.cellanims[i].object->getAnimations();
                if (value < animations.size()) {
                    auto& animation = session.cellanims[i].object->getAnimation(value);

                    // +1 because of the trailing underscore.
                    animation.name = key.substr(cellanimNameLen + 1);
                    if (comment != "(null)")
                        animation.comment = comment;
                }
            }
        }
    }

    // Sheets.
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

    // Editor data.
    const Archive::File* tedSearch = Archive::findFile(EditorDataProc::ARCHIVE_FILENAME, *rootDirIt);
    if (tedSearch) {
        EditorDataProc::Apply(session, tedSearch->data.data(), tedSearch->data.size());
    }

    return true;
}

static bool InitCtrSession(Session& session, const Archive::SARCObject& archive) {
    // Every layout archive has the directory "blyt". If this directory exists
    // we should throw an error.
    auto blytDirIt = std::find_if(
        archive.getStructure().subdirectories.begin(),
        archive.getStructure().subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "blyt"; }
    );

    if (blytDirIt != archive.getStructure().subdirectories.end()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The selected file is a layout archive; please choose a cellanim archive instead."
        ));
        return false;
    }

    auto rootDirIt = std::find_if(
        archive.getStructure().subdirectories.begin(),
        archive.getStructure().subdirectories.end(),
        [](const Archive::Directory& dir) { return dir.name == "arc"; }
    );

    if (rootDirIt == archive.getStructure().subdirectories.end()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The archive does not contain the root directory: are you sure this is a cellanim archive?"
        ));
        return false;
    }

    std::vector<const Archive::File*> bccadFiles;
    for (const auto& file : rootDirIt->files) {
        if (
            file.name.size() >= STR_LIT_LEN(".bccad") &&
            file.name.substr(file.name.size() - STR_LIT_LEN(".bccad")) == ".bccad"
        )
            bccadFiles.push_back(&file);
    }

    if (bccadFiles.empty()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The archive does not contain any cellanim data files (.bccad): are you\n"
            "sure this is a cellanim archive?"
        ));
        return false;
    }

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

        if (!foundMatch) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "One or more cellanim data files (.bccad) could not be matched with a\n"
                "texture file (.ctpk); one may be missing."
            ));
            return false;
        }
    }

    session.cellanims.resize(bccadFiles.size());
    session.sheets->getVector().reserve(ctpkFiles.size());

    // Cellanims.
    for (size_t i = 0; i < bccadFiles.size(); i++) {
        auto& cellanim = session.cellanims[i];
        const auto* file = bccadFiles[i];

        cellanim.object = std::make_shared<CellAnim::CellAnimObject>(
            file->data.data(), file->data.size()
        );
        cellanim.object->setName(file->name.substr(0, file->name.size() - STR_LIT_LEN(".bccad")));

        if (!cellanim.object->isInitialized()) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "A cellanim data file (.bccad) could not be deserialized; it might be corrupted."
            ));
            return false;
        }

        if (cellanim.object->getType() != CellAnim::CELLANIM_TYPE_CTR) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "A cellanim data file (.bccad) is in the wrong format (expected 3DS, got Wii)."
            ));
            return false;
        }

        cellanim.object->setSheetIndex(i);
    }

    // Sheets.
    // Note: this is wrapped in a MainThreadTask since we need to get access to the
    //       GL context to create GPU textures. This can be outside of a MainThreadTask
    //       but then it would use multiple MainThreadTasks instead of just one.
    bool didError = false;

    MainThreadTaskManager::getInstance().QueueTask([&didError, &ctpkFiles, &session]() {
        for (size_t i = 0; i < ctpkFiles.size(); i++) {
            const auto* file = ctpkFiles[i];

            CTPK::CTPKObject ctpkObject = CTPK::CTPKObject(
                file->data.data(), file->data.size()
            );
            if (!ctpkObject.isInitialized()) {
                PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                    std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                    "A texture file (.ctpk) could not be deserialized; it might be corrupted."
                ));
                didError = true;
                return;
            }

            if (ctpkObject.mTextures.empty()) {
                PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                    std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                    "No textures were found in a texture file (.ctpk) when at least one was expected."
                ));
                didError = true;
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

    if (didError)
        return false;

    // Editor data.
    const Archive::File* tedSearch = Archive::findFile(EditorDataProc::ARCHIVE_FILENAME, *rootDirIt);
    if (tedSearch) {
        EditorDataProc::Apply(session, tedSearch->data.data(), tedSearch->data.size());
    }

    return true;
}

ssize_t SessionManager::CreateSession(std::string_view filePath) {
    if (!FileUtil::doesFileExist(filePath)) {
        Logging::err << "[SessionManager::CreateSession] File does not exist: " << filePath << std::endl;

        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The specified file could not be opened because it does not exist."
        ));

        return -1;
    }

    Logging::info <<
        "[SessionManager::CreateSession] Creating session from path \"" << filePath << "\".." << std::endl;

    std::vector<unsigned char> data;

    auto _data = FileUtil::openFileData(filePath);
    if (!_data.has_value()) {
        Logging::err << "[SessionManager::CreateSession] Error opening file at path: " << filePath << std::endl;

        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The specified file could not be opened; do you have read permissions?"
        ));

        return -1;
    }

    data = std::move(*_data);

    CellAnim::CellAnimType type { CellAnim::CELLANIM_TYPE_INVALID };

    // We check for Yaz0 first since it has a magic value.
    if (Yaz0::checkDataValid(data.data(), data.size())) {
        type = CellAnim::CELLANIM_TYPE_RVL;

        const auto decompressedData = Yaz0::decompress(data.data(), data.size());
        if (!decompressedData.has_value()) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "The archive data could not be decompressed; it might be corrupted."
            ));
            return -1;
        }

        data = std::move(*decompressedData);
    }
    else if (NZlib::checkDataValid(data.data(), data.size())) {
        type = CellAnim::CELLANIM_TYPE_CTR;

        const auto decompressedData = NZlib::decompress(data.data(), data.size());
        if (!decompressedData.has_value()) {
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "The archive data could not be decompressed; it might be corrupted."
            ));
            return -1;
        }

        data = std::move(*decompressedData);
    }
    else {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(CREATE_SESSION_ERR_POPUP_TITLE),
            "The compressed data is invalid: are you sure this is a cellanim archive?"
        ));
        return -1;
    }

    bool initOk = false;
    Session newSession;

    switch (type) {
    case CellAnim::CELLANIM_TYPE_RVL: {
        Archive::DARCHObject archive = Archive::DARCHObject(
            data.data(), data.size()
        );

        if (!archive.isInitialized()) {
            initOk = false;
            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                "The archive data could not be deserialized: are you sure this is a cellanim archive?"
            ));
            break;
        }

        initOk = InitRvlSession(newSession, archive);
    } break;
    case CellAnim::CELLANIM_TYPE_CTR: {
        Archive::SARCObject archive = Archive::SARCObject(data.data(), data.size());

        if (!archive.isInitialized()) {
            initOk = false;

            const uint32_t observedMagic = *reinterpret_cast<const uint32_t*>(data.data());
            switch (observedMagic) {
            case IDENTIFIER_TO_U32('C','G','F','X'):
                PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                    std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                    "The selected file is a BCRES; please choose a cellanim archive instead."
                ));
                break;
            case IDENTIFIER_TO_U32('S','P','B','D'):
                PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                    std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                    "The selected file is an effect file; please choose a cellanim archive instead."
                ));
                break;

            default:
                PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                    std::string(CREATE_SESSION_ERR_POPUP_TITLE),
                    "The archive data could not be deserialized: are you sure this is a cellanim archive?"
                ));
                break;
            }

            break;
        }

        initOk = InitCtrSession(newSession, archive);
    } break;

    default:
        throw std::runtime_error("SessionManager::CreateSession: invalid type; this shouldn't be reached");
    }

    if (!initOk) {
        return -1;
    }

    newSession.type = type;

    newSession.resourcePath = filePath;
    newSession.setCurrentCellAnimIndex(0);

    std::lock_guard<std::mutex> lock(mMtx);

    mSessions.push_back(std::move(newSession));
    const ssize_t sessionIndex = static_cast<ssize_t>(mSessions.size()) - 1;

    Logging::info <<
        "[SessionManager::CreateSession] Created session no. " << sessionIndex + 1 << '.' << std::endl;

    return sessionIndex;
}

static bool SerializeRvlSession(const Session& session, std::vector<unsigned char>& output) {
    Archive::DARCHObject archive;

    archive.getStructure().AddDirectory(Archive::Directory("."));
    auto& directory = archive.getStructure().subdirectories.back();

    // BRCAD files
    for (size_t i = 0; i < session.cellanims.size(); i++) {
        const auto& cellanim = session.cellanims[i];

        Archive::File file(cellanim.object->getName() + ".brcad");

        Logging::info <<
            "[SerializeRvlSession] Serializing cellanim \"" << cellanim.object->getName() << "\".." << std::endl;

        const auto& sheet = session.sheets->getTextureByIndex(session.cellanims[i].object->getSheetIndex());

        // Make sure usePalette is synced.
        session.cellanims[i].object->setUsePalette(TPL::getImageFormatPaletted(sheet->getTPLOutputFormat()));

        file.data = session.cellanims[i].object->Serialize();

        directory.AddFile(std::move(file));
    }

    // Header files
    for (size_t i = 0; i < session.cellanims.size(); i++) {
        const std::string& cellanimName = session.cellanims[i].object->getName();

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

            stream <<
                "#define " << cellanimName << '_' << animation.name << '\t' << std::to_string(j) <<
                "\t// " << (animation.comment.empty() ? "(null)" : animation.comment) << "\r\n";
        }

        const std::string strUtf8 = stream.str();
        const std::string strShiftJIS = ShiftJISUtil::convertToShiftJIS(strUtf8.c_str(), strUtf8.length());

        file.data.insert(file.data.end(), strShiftJIS.begin(), strShiftJIS.end());

        directory.AddFile(std::move(file));
    }

    // TPL file
    {
        Archive::File file("cellanim.tpl");

        TPL::TPLObject tplObject;
        bool didError = false;

        MainThreadTaskManager::getInstance().QueueTask([&session, &tplObject, &didError]() {
            tplObject.mTextures.resize(session.sheets->getTextureCount());
            for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
                const auto& sheet = session.sheets->getTextureByIndex(i);

                auto tplTexture = sheet->TPLTexture();
                if (!tplTexture.has_value()) {
                    PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                        std::string(EXPORT_SESSION_ERR_POPUP_TITLE),
                        "An error occurred when serializing the texture file; please check the log\n"
                        "for more details."
                    ));

                    didError = true;
                    return;
                }

                tplObject.mTextures[i] = std::move(*tplTexture);
            }
        }).get();

        if (didError)
            return false;

        Logging::info << "[SerializeRvlSession] Serializing textures.." << std::endl;

        file.data = tplObject.Serialize();

        directory.AddFile(std::move(file));
    }

    // TED file
    {
        Archive::File file { std::string(EditorDataProc::ARCHIVE_FILENAME) };

        file.data = EditorDataProc::Create(session);

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

    if (!compressedArchive.has_value()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(EXPORT_SESSION_ERR_POPUP_TITLE),
            "An error occurred when compressing the archive; please check the log for\n"
            "more details."
        ));
        return false;
    }

    output = std::move(*compressedArchive);
    compressedArchive.reset();

    return true;
}

static bool SerializeCtrSession(const Session& session, std::vector<unsigned char>& output) {
    Archive::SARCObject archive;

    archive.getStructure().AddDirectory(Archive::Directory("arc"));
    auto& directory = archive.getStructure().subdirectories.back();

    // BCCAD files
    for (size_t i = 0; i < session.cellanims.size(); i++) {
        const auto& cellanim = session.cellanims[i];

        Archive::File file(cellanim.object->getName() + ".bccad");

        Logging::info <<
            "[SerializeCtrSession] Serializing cellanim \"" << cellanim.object->getName() << "\".." << std::endl;

        file.data = cellanim.object->Serialize();

        directory.AddFile(std::move(file));
    }

    // CTPK files
    std::vector<CTPK::CTPKTexture> ctpkTextures;
    bool didError = false;

    MainThreadTaskManager::getInstance().QueueTask([&session, &ctpkTextures, &didError]() {
        ctpkTextures.resize(session.sheets->getTextureCount());
        for (unsigned i = 0; i < session.sheets->getTextureCount(); i++) {
            const auto& sheet = session.sheets->getTextureByIndex(i);

            auto ctpkTexture = sheet->CTPKTexture();
            if (!ctpkTexture.has_value()) {
                PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                    std::string(EXPORT_SESSION_ERR_POPUP_TITLE),
                    "An error occurred when serializing a texture file; please check the log\n"
                    "for more details."
                ));

                didError = true;
                return;
            }

            ctpkTextures[i] = std::move(*ctpkTexture);
        }
    }).get();

    if (didError)
        return false;

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
        Archive::File file { std::string(EditorDataProc::ARCHIVE_FILENAME) };

        file.data = EditorDataProc::Create(session);

        directory.AddFile(std::move(file));
    }

    Logging::info << "[SerializeCtrSession] Serializing archive.." << std::endl;

    auto archiveBinary = archive.Serialize();

    Logging::info << "[SerializeCtrSession] Compressing archive.." << std::endl;

    auto compressedArchive = NZlib::compress(
        archiveBinary.data(), archiveBinary.size(),
        ConfigManager::getInstance().getConfig().compressionLevel
    );

    if (!compressedArchive.has_value()) {
        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(EXPORT_SESSION_ERR_POPUP_TITLE),
            "An error occurred when compressing the archive; please check the log for\n"
            "more details."
        ));
        return false;
    }

    output = std::move(*compressedArchive);
    compressedArchive.reset();

    return true;
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

    bool initOk = false;
    std::vector<unsigned char> result;

    switch (session.type) {
    case CellAnim::CELLANIM_TYPE_RVL:
        initOk = SerializeRvlSession(session, result);
        break;
    case CellAnim::CELLANIM_TYPE_CTR:
        initOk = SerializeCtrSession(session, result);
        break;

    default:
        throw std::runtime_error("SessionManager::ExportSession: invalid type; this shouldn't be reached");
    }

    if (!initOk)
        return false;

    const ConfigManager& configManager = ConfigManager::getInstance();

    if (
        FileUtil::doesFileExist(dstFilePath) && 
        configManager.getConfig().backupBehaviour != BackupBehaviour::None
    ) {
        std::string backupPath = std::string(dstFilePath) + ".bak";

        Logging::info << "[SessionManager::ExportSession] Backing up file at \"" 
                    << dstFilePath << "\" to \"" << backupPath << "\".." << std::endl;

        bool success = FileUtil::copyFile(
            dstFilePath, backupPath,
            configManager.getConfig().backupBehaviour == BackupBehaviour::SaveOverwrite
        );

        if (!success) {
            Logging::err << "[SessionManager::ExportSession] Failed to save backup of file! Aborting.." << std::endl;

            PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
                std::string(EXPORT_SESSION_ERR_POPUP_TITLE),
                "The output file could not be backed up; do you have file creation and/or\n"
                "writing permissions?"
            ));

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
        Logging::err << "[SessionManager::ExportSession] Could not open output file! Aborting.." << std::endl;

        PromptPopupManager::getInstance().Queue(PromptPopupManager::CreatePrompt(
            std::string(EXPORT_SESSION_ERR_POPUP_TITLE),
            "The output file could not be opened for writing; do you have file creation\n"
            "and/or writing permissions?"
        ));

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
            static_cast<ssize_t>(mSessions.size()) - 1
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
