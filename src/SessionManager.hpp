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

constexpr unsigned int SESSION_MAX_COMMANDS = 512;

class Session {
public:
    Session() = default;
    ~Session() = default;

    Session(Session&& other) noexcept
        : cellanims(std::move(other.cellanims)),
          sheets(std::move(other.sheets)),
          currentCellanim(other.currentCellanim),
          mainPath(std::move(other.mainPath)),
          traditionalMethod(other.traditionalMethod),
          imagePath(std::move(other.imagePath)),
          headerPath(std::move(other.headerPath)),
          arrangementMode(other.arrangementMode),
          modified(other.modified),
          undoQueue(std::move(other.undoQueue)),
          redoQueue(std::move(other.redoQueue)) {}

    Session& operator=(Session&& other) noexcept {
        if (this != &other) {
            cellanims = std::move(other.cellanims);
            sheets = std::move(other.sheets);
            currentCellanim = other.currentCellanim;
            mainPath = std::move(other.mainPath);
            traditionalMethod = other.traditionalMethod;
            imagePath = std::move(other.imagePath);
            headerPath = std::move(other.headerPath);
            arrangementMode = other.arrangementMode;
            modified = other.modified;
            undoQueue = std::move(other.undoQueue);
            redoQueue = std::move(other.redoQueue);
        }
        return *this;
    }

public:
    struct CellanimData {
        std::shared_ptr<RvlCellAnim::RvlCellAnimObject> object;
        std::string name;
    };

public:
    void addCommand(std::shared_ptr<BaseCommand> command);

    void undo();
    void redo();

    bool canUndo() const { return !this->undoQueue.empty(); }
    bool canRedo() const { return !this->redoQueue.empty(); }

    void clearUndoRedo() { this->undoQueue.clear(); this->redoQueue.clear(); };

    CellanimData& getCurrentCellanim() {
        return this->cellanims.at(this->currentCellanim);
    }

    std::shared_ptr<Texture>& getCurrentCellanimSheet() {
        return this->sheets.at(std::min<unsigned>(
            this->cellanims.at(this->currentCellanim).object->sheetIndex,
            this->sheets.size() - 1
        ));
    }

public:
    std::vector<CellanimData> cellanims;
    std::vector<std::shared_ptr<Texture>> sheets;

    unsigned currentCellanim { 0 };

    std::string mainPath;

    bool traditionalMethod { false }; // Has this session been opened with separate files on the FS?
    std::unique_ptr<std::string> imagePath;
    std::unique_ptr<std::string> headerPath;

    bool arrangementMode { false };

    bool modified { false };

private:
    std::deque<std::shared_ptr<BaseCommand>> undoQueue;
    std::deque<std::shared_ptr<BaseCommand>> redoQueue;
};

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>; // Allow access to base class constructor

private:
    SessionManager() = default;
public:
    ~SessionManager() = default;

public:
    Session* getCurrentSession() {
        if (this->currentSessionIndex >= 0)
            return &this->sessionList.at(this->currentSessionIndex);

        return nullptr;
    }

    bool getCurrentSessionModified() const {
        if (this->currentSessionIndex >= 0)
            return this->sessionList.at(this->currentSessionIndex).modified;
        else
            return false;
    }
    void setCurrentSessionModified(bool modified) {
        if (this->currentSessionIndex >= 0)
            this->sessionList.at(this->currentSessionIndex).modified = modified;
    }

    bool getSessionAvaliable() const {
        return this->currentSessionIndex >= 0;
    }

    void SessionChanged();

    // Push session from a Yaz0-compressed U8 archive (SZS).
    int PushSessionFromCompressedArc(const char* filePath);

    int ExportSessionCompressedArc(Session* session, const char* outPath);

    void ClearSessionPtr(Session* session);
    void FreeSessionIndex(int index);

    void FreeAllSessions();

public:
    enum Error {
        Error_None = 0,

        OpenError_FileDoesNotExist = -0xFF,
        OpenError_FailOpenArchive,
        OpenError_FailFindTPL,
        OpenError_RootDirNotFound,
        OpenError_NoBXCADsFound,
        OpenError_FailOpenBXCAD,
        OpenError_FailOpenTPL,
        OpenError_LayoutArchive, // The user selected a layout archive instead of a cellanim archive.

        OutError_FailOpenFile,
        OutError_ZlibError,
        OutError_FailTPLTextureExport,
    };

    Error currentError { Error_None };

    std::deque<Session> sessionList;

    int currentSessionIndex { -1 };
    // Current session being closed. Used for closing while modified warning.
    int sessionClosingIndex { -1 };

private:
    std::mutex mtx;
};

#endif // SESSIONMANAGER_HPP
