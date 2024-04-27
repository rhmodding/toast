#include "SessionManager.hpp"

#include "archive/U8.hpp"
#include "texture/TPL.hpp"

#include <sstream>
#include <fstream>

#include "AppState.hpp"

#include "common.hpp"

#define GL_SILENCE_DEPRECATION

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

int16_t SessionManager::firstFreeSessionIndex() {
    int16_t index = -1;
    for (int16_t i = 0; i < ARRAY_LENGTH(this->sessions); ++i) {
        if (this->sessions[i].open == false) {
            index = i;
            break;
        }
    }

    return index;
}

int16_t SessionManager::PushSessionFromArc(const char* arcPath) {
    int16_t index = this->firstFreeSessionIndex();
    if (index < 0) {
        this->lastSessionError = SessionOpenError_SessionsFull;
        return -1;
    }

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

    this->sessions[index].open = true;
    this->sessions[index].mainPath = arcPath;
    
    this->sessions[index].animationNames = animationNames;
    this->sessions[index].cellanims = cellanims;
    this->sessions[index].cellanimSheets = cellanimSheets;

    for (auto brcad : brcadFiles)
        this->sessions[index].cellNames.push_back(brcad->name);

    this->lastSessionError = SessionOpenError_None;

    return index;
}

int16_t SessionManager::PushSessionTraditional(const char* paths[3]) {
    int16_t index = this->firstFreeSessionIndex();
    if (index < 0) {
        this->lastSessionError = SessionOpenError_SessionsFull;
        return -1;
    }

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

    this->sessions[index].open = true;
    this->sessions[index].mainPath = paths[0];

    this->sessions[index].pngPath = new std::string(paths[1]);
    this->sessions[index].headerPath = new std::string(paths[2]);

    this->sessions[index].animationNames.push_back(animationNames);
    this->sessions[index].cellanims.push_back(cellanim);
    this->sessions[index].cellanimSheets.push_back(image);

    return index;
}

void SessionManager::FreeSession(Session* session) {
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

    Common::deleteIfNotNullptr(session->pngPath);
    Common::deleteIfNotNullptr(session->headerPath);

    session->open = false;
}

void SessionManager::FreeAllSessions() {
    for (uint16_t i = 0; i < ARRAY_LENGTH(this->sessions); i++) {
        if (this->sessions[i].open)
            this->FreeSession(&this->sessions[i]);
    }
}

void SessionManager::SessionChanged() {
    if (this->currentSession >= 0) {
        GET_APP_STATE;
        GET_ANIMATABLE;

        Common::deleteIfNotNullptr(globalAnimatable);

        globalAnimatable = new Animatable(
            this->sessions[this->currentSession].getCellanim(),
            this->sessions[this->currentSession].getCellanimSheet()
        );
        
        globalAnimatable->setAnimation(0);
        appState.playerState.updateSetFrameCount();

        appState.selectedAnimation = 0;
        appState.selectedPart = -1;
    }
}