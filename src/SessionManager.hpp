#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include "texture/Texture.hpp"

#include "anim/RvlCellAnim.hpp"

#include "command/BaseCommand.hpp"

#include <cstdint>

#include <string>

#include <unordered_map>
#include <deque>

#include <memory>

#include <mutex>

#include "common.hpp"

#define SESSION_MAX_COMMANDS 512

// Stores instance of SessionManager in local sessionManager.
#define GET_SESSION_MANAGER SessionManager& sessionManager = SessionManager::getInstance()

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>; // Allow access to base class constructor

private:
    SessionManager() = default;
public:
    ~SessionManager() = default;

private:
    std::mutex mtx;

public:
    struct Session {
        struct CellanimData {
            std::shared_ptr<RvlCellAnim::RvlCellAnimObject> object;
            std::string name;
            std::unordered_map<unsigned, std::string> animNames;
        };
        std::vector<CellanimData> cellanims;
        std::vector<std::shared_ptr<Texture>> sheets;

        unsigned currentCellanim { 0 };

        ///////////////////////////

        std::string mainPath;

        bool traditionalMethod { false }; // Has this session been opened with separate files on the FS?
        std::unique_ptr<std::string> imagePath;
        std::unique_ptr<std::string> headerPath;

        ///////////////////////////

        std::shared_ptr<RvlCellAnim::RvlCellAnimObject> getCellanimObject() {
            return this->cellanims.at(this->currentCellanim).object;
        }
        std::string& getCellanimName() {
            return this->cellanims.at(this->currentCellanim).name;
        }
        std::shared_ptr<Texture>& getCellanimSheet() {
            return this->sheets.at(std::min<unsigned>(this->getCellanimObject()->sheetIndex, this->sheets.size() - 1));
        }
        std::unordered_map<unsigned, std::string>& getAnimationNames() {
            return this->cellanims.at(this->currentCellanim).animNames;
        }

        bool arrangementMode { false };

        bool modified { false };

    private:
        std::deque<std::shared_ptr<BaseCommand>> undoQueue;
        std::deque<std::shared_ptr<BaseCommand>> redoQueue;

    public:
        void executeCommand(std::shared_ptr<BaseCommand> command);

        void undo();
        void redo();

        bool canUndo() { return !this->undoQueue.empty(); }
        bool canRedo() { return !this->redoQueue.empty(); }

        void clearUndoRedo();
    };
    
    enum Error {
        Error_None = 0,

        OpenError_FileDoesNotExist = -0xFF,
        OpenError_FailOpenArchive,
        OpenError_FailFindTPL,
        OpenError_RootDirNotFound,
        OpenError_NoBXCADsFound,
        OpenError_FailOpenBXCAD,
        OpenError_FailOpenTPL,
        OpenError_FailOpenImage,
        OpenError_FailOpenHFile,
        OpenError_LayoutArchive, // The user selected a layout archive instead of a cellanim archive.
        OpenError_SessionsFull,

        OutError_FailOpenFile,
        OutError_ZlibError,
        OutError_FailTPLTextureExport,
    };

public:
    std::deque<Session> sessionList;

    int currentSessionIndex { -1 };
    // Current session being closed. Used for closing while modified warning.
    int sessionClosingIndex { -1 };

    Error currentError { Error_None };

    Session* getCurrentSession() {
        if (this->currentSessionIndex >= 0)
            return &this->sessionList.at(this->currentSessionIndex);

        return nullptr;
    }

    bool& getCurrentSessionModified() {
        return this->getCurrentSession()->modified;
    }

    bool getSessionAvaliable() {
        return this->currentSessionIndex >= 0;
    }

    void SessionChanged();

    // Push session from a Yaz0-compressed U8 archive (SZS).
    int PushSessionFromCompressedArc(const char* filePath);
    // Push session from separated files on FS.
    int PushSessionTraditional(const char* brcadPath, const char* imagePath, const char* headerPath);

    int ExportSessionCompressedArc(Session* session, const char* outPath);

    void ClearSessionPtr(Session* session);
    void FreeSessionIndex(int index);

    void FreeAllSessions();
};

#endif // SESSIONMANAGER_HPP
