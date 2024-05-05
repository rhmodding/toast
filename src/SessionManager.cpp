#include "SessionManager.hpp"

#include "archive/U8.hpp"

#include "compression/Yaz0.hpp"

#include <sstream>
#include <fstream>

#include "AppState.hpp"

#include "common.hpp"

GLuint LoadTPLTextureIntoGLTexture(TPL::TPLTexture tplTexture) {
    GLuint imageTexture;
    
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);

    GLint minFilter;
    GLint magFilter;

    switch (tplTexture.magFilter) {
        case TPL::TPL_TEX_FILTER_NEAR:
            magFilter = GL_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_LINEAR:
            magFilter = GL_LINEAR;
            break;
        default:
            magFilter = GL_LINEAR;
            break;
    }

    switch (tplTexture.minFilter) {
        case TPL::TPL_TEX_FILTER_NEAR:
            minFilter = GL_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_LINEAR:
            minFilter = GL_LINEAR;
            break;
        case TPL::TPL_TEX_FILTER_NEAR_MIP_NEAR:
            minFilter = GL_NEAREST_MIPMAP_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_LIN_MIP_NEAR:
            minFilter = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case TPL::TPL_TEX_FILTER_NEAR_MIP_LIN:
            minFilter = GL_NEAREST_MIPMAP_LINEAR;
            break;
        case TPL::TPL_TEX_FILTER_LIN_MIP_LIN:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            break;
        default:
            minFilter = GL_LINEAR;
            break;
    }

    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        tplTexture.wrapS == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
    );
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        tplTexture.wrapT == TPL::TPL_WRAP_MODE_REPEAT ? GL_REPEAT : GL_CLAMP
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tplTexture.width, tplTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return imageTexture;
}

int32_t SessionManager::PushSessionFromArc(const char* arcPath) {
    Session newSession;

    auto archiveResult = U8::readYaz0U8Archive(arcPath);
    if (!archiveResult.has_value()) {
        this->lastSessionError = SessionOpenError_FailOpenArchive;
        return -1;
    }

    U8::U8ArchiveObject* archiveObject = &archiveResult.value();

    if (archiveObject->structure.subdirectories.size() < 1) {
        this->lastSessionError = SessionOpenError_RootDirNotFound;
        return -1;
    }

    auto tplSearch = U8::findFile("./cellanim.tpl", archiveObject->structure);
    if (!tplSearch.has_value()) {
        this->lastSessionError = SessionOpenError_FailFindTPL;
        return -1;
    }

    U8::File* tplFile = &tplSearch.value();
    TPL::TPLObject tplObject =
        TPL::TPLObject(tplFile->data.data(), tplFile->data.size());

    std::vector<const U8::File*> brcadFiles;
    for (const auto& file : archiveObject->structure.subdirectories.at(0).files) {
        if (file.name.size() >= 6 && file.name.substr(file.name.size() - 6) == ".brcad") {
            brcadFiles.push_back(&file);
        }
    }

    if (brcadFiles.size() < 1) {
        this->lastSessionError = SessionOpenError_NoBXCADsFound;
        return -1;
    }

    std::vector<std::unordered_map<uint16_t, std::string>*> animationNames(brcadFiles.size());
    std::vector<RvlCellAnim::RvlCellAnimObject*> cellanims(brcadFiles.size());
    std::vector<Common::Image*> cellanimSheets(brcadFiles.size());

    // animationNames
    for (uint16_t i = 0; i < brcadFiles.size(); i++) {
        const U8::File* headerFile{ nullptr };
        std::string targetHeaderName =
            "rcad_" +
            brcadFiles.at(i)->name.substr(0, brcadFiles.at(i)->name.size() - 6) +
            "_labels.h";

        for (const auto& file : archiveObject->structure.subdirectories.at(0).files) {
            if (file.name == targetHeaderName) {
                headerFile = &file;
                continue;
            }
        }

        if (!headerFile)
            continue;

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

                if (!animationNames.at(i))
                    animationNames.at(i) = new std::unordered_map<uint16_t, std::string>;

                animationNames.at(i)->insert({ value, key });
            }
        }
    }

    // cellanims
    for (uint16_t i = 0; i < brcadFiles.size(); i++)
        cellanims.at(i) = new RvlCellAnim::RvlCellAnimObject(brcadFiles.at(i)->data.data(), brcadFiles.at(i)->data.size());

    // cellanimSheets
    for (uint16_t i = 0; i < brcadFiles.size(); i++)
        cellanimSheets.at(i) = new Common::Image(
            tplObject.textures.at(cellanims.at(i)->sheetIndex).width,
            tplObject.textures.at(cellanims.at(i)->sheetIndex).height,
            LoadTPLTextureIntoGLTexture(tplObject.textures.at(cellanims.at(i)->sheetIndex))
        );

    newSession.open = true;
    newSession.mainPath = arcPath;
    
    newSession.animationNames = animationNames;
    newSession.cellanims = cellanims;
    newSession.cellanimSheets = cellanimSheets;

    for (auto brcad : brcadFiles)
        newSession.cellNames.push_back(brcad->name);

    this->sessionList.push_back(newSession);

    this->lastSessionError = SessionOpenError_None;

    return static_cast<int32_t>(this->sessionList.size() - 1);
}

int32_t SessionManager::PushSessionTraditional(const char* paths[3]) {
    Session newSession;

    RvlCellAnim::RvlCellAnimObject* cellanim = RvlCellAnim::ObjectFromFile(paths[0]);
    if (!cellanim) {
        this->lastSessionError = SessionOpenError_FailOpenBXCAD;
        return -1;
    }

    Common::Image* image = new Common::Image;
    image->LoadFromFile(paths[1]);

    if (!image->texture) {
        this->lastSessionError = SessionOpenError_FailOpenPNG;
        return -1;
    }

    std::unordered_map<uint16_t, std::string>* animationNames =
        new std::unordered_map<uint16_t, std::string>;
    
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

    newSession.open = true;
    newSession.mainPath = paths[0];

    newSession.pngPath = new std::string(paths[1]);
    newSession.headerPath = new std::string(paths[2]);

    newSession.animationNames.push_back(animationNames);
    newSession.cellanims.push_back(cellanim);
    newSession.cellanimSheets.push_back(image);

    this->sessionList.push_back(newSession);

    this->lastSessionError = SessionOpenError_None;

    return static_cast<int32_t>(this->sessionList.size() - 1);
}

int32_t SessionManager::ExportSessionArc(Session* session, const char* outPath) {
    U8::U8ArchiveObject archive;

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
            glBindTexture(GL_TEXTURE_2D, session->cellanimSheets.at(i)->texture);

            TPL::TPLTexture tplTexture;
            tplTexture.data.resize(
                session->cellanimSheets.at(i)->width *
                session->cellanimSheets.at(i)->height *
                4
            );

            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tplTexture.data.data());

            tplTexture.width = session->cellanimSheets.at(i)->width;
            tplTexture.height = session->cellanimSheets.at(i)->height;

            GLint wrapModeS, wrapModeT;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapModeS);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrapModeT);

            tplTexture.wrapS = wrapModeS == GL_REPEAT ?
                TPL::TPL_WRAP_MODE_REPEAT :
                TPL::TPL_WRAP_MODE_CLAMP;
            tplTexture.wrapT = wrapModeT == GL_REPEAT ?
                TPL::TPL_WRAP_MODE_REPEAT :
                TPL::TPL_WRAP_MODE_CLAMP;

            tplTexture.minFilter = TPL::TPL_TEX_FILTER_LINEAR;
            tplTexture.magFilter = TPL::TPL_TEX_FILTER_LINEAR;

            tplTexture.mipMap = 1;

            tplTexture.valid = 0xFFu;

            tplObject.textures.push_back(tplTexture);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        file.data = tplObject.Reserialize();

        directory.AddFile(file);
    }

    archive.structure.AddDirectory(directory);

    auto archiveRaw = archive.Reserialize();
    auto compressedArchive = Yaz0::compress(archiveRaw.data(), archiveRaw.size());

    if (!compressedArchive.has_value()) {
        this->lastSessionError = SessionOutError_ZlibError;
        return -1;
    }

    std::ofstream file(outPath, std::ios::binary);

    if (file.is_open()) {
        file.write(
            reinterpret_cast<const char*>(compressedArchive.value().data()),
            compressedArchive.value().size()
        );
        
        file.close();
    } else {
        this->lastSessionError = SessionOutError_FailOpenFile;
        return -1;
    }

    return 0;
}

void SessionManager::ClearSessionPtr(Session* session) {
    for (auto animationNameList : session->animationNames)
        delete animationNameList;
    for (auto cellanim : session->cellanims)
        delete cellanim;
    for (auto cellanimSheet : session->cellanimSheets)
        delete cellanimSheet;

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
        
        globalAnimatable->setAnimation(0);
        appState.playerState.updateSetFrameCount();

        appState.selectedAnimation = 0;
        appState.selectedPart = -1;
    }
}