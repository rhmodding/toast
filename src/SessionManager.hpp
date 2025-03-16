#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include "texture/Texture.hpp"
#include "texture/TextureGroup.hpp"

#include "cellanim/CellAnim.hpp"

#include "command/BaseCommand.hpp"

#include <string>

#include <deque>

#include <memory>

#include <mutex>

class Session {
public:
    Session() = default;
    ~Session() = default;

    Session(Session&& other) noexcept :
        cellanims(std::move(other.cellanims)),
        sheets(std::move(other.sheets)),
        resourcePath(std::move(other.resourcePath)),
        arrangementMode(other.arrangementMode),
        modified(other.modified),
        type(other.type),
        undoQueue(std::move(other.undoQueue)),
        redoQueue(std::move(other.redoQueue)),
        currentCellanim(other.currentCellanim)
    {}

    Session& operator=(Session&& other) noexcept {
        if (this != &other) {
            this->cellanims = std::move(other.cellanims);
            this->sheets = std::move(other.sheets);
            this->resourcePath = std::move(other.resourcePath);
            this->arrangementMode = other.arrangementMode;
            this->modified = other.modified;
            this->type = other.type;
            this->undoQueue = std::move(other.undoQueue);
            this->redoQueue = std::move(other.redoQueue);
            this->currentCellanim = other.currentCellanim;
        }
        return *this;
    }

public:
    struct CellanimData {
        std::shared_ptr<CellAnim::CellAnimObject> object;
        std::string name;
    };

public:
    void addCommand(std::shared_ptr<BaseCommand> command);

    void undo();
    void redo();

    bool canUndo() const { return !this->undoQueue.empty(); }
    bool canRedo() const { return !this->redoQueue.empty(); }

    void clearUndoRedo() { this->undoQueue.clear(); this->redoQueue.clear(); };

    unsigned getCurrentCellanimIndex() const { return this->currentCellanim; }
    void setCurrentCellanimIndex(unsigned index);

    CellanimData& getCurrentCellanim() {
        return this->cellanims.at(this->currentCellanim);
    }

    std::shared_ptr<Texture>& getCurrentCellanimSheet() {
        return this->sheets->getTextureByIndex(
            this->cellanims.at(this->currentCellanim).object->sheetIndex
        );
    }

public:
    static constexpr unsigned int COMMANDS_MAX = 512;

    std::vector<CellanimData> cellanims;
    std::shared_ptr<TextureGroup> sheets { std::make_shared<TextureGroup>() };

    std::string resourcePath;

    bool arrangementMode { false };

    bool modified { false };

    CellAnim::CellAnimType type;

private:
    std::deque<std::shared_ptr<BaseCommand>> undoQueue;
    std::deque<std::shared_ptr<BaseCommand>> redoQueue;

    unsigned currentCellanim { 0 };
};

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>; // Allow access to base class constructor

private:
    SessionManager() = default;
public:
    ~SessionManager() = default;

private:
    void onSessionChange();

public:
    Session* getCurrentSession() {
        if (this->currentSessionIndex >= 0)
            return &this->sessions.at(this->currentSessionIndex);

        return nullptr;
    }

    bool getCurrentSessionModified() const {
        if (this->currentSessionIndex >= 0)
            return this->sessions.at(this->currentSessionIndex).modified;
        else
            return false;
    }
    void setCurrentSessionModified(bool modified) {
        if (this->currentSessionIndex >= 0)
            this->sessions.at(this->currentSessionIndex).modified = modified;
    }

    bool getSessionAvaliable() const {
        return this->currentSessionIndex >= 0;
    }

    int  getCurrentSessionIndex() const { return this->currentSessionIndex; }
    void setCurrentSessionIndex(int sessionIndex);

    // Create a new session from the path of a cellanim archive (.szs).
    //
    // Returns: index of new session if succeeded, -1 if failed
    int CreateSession(const char* filePath);

    // Export a session as a cellanim archive (.szs) to the specified path.
    // Note: if dstFilePath is NULL, then the session's resourcePath is used.
    //
    // Returns: true if succeeded, false if failed
    bool ExportSession(unsigned sessionIndex, const char* dstFilePath = nullptr);

    // Remove a session by it's index.
    void RemoveSession(unsigned sessionIndex);

    // Remove all sessions.
    void RemoveAllSessions();

public:
    enum Error {
        Error_None = 0,

        OpenError_FileDoesNotExist = -0xFF,
        OpenError_FailOpenFile,
        OpenError_FailOpenArchive,
        OpenError_FailFindTPL,
        OpenError_RootDirNotFound,
        OpenError_NoBXCADsFound,
        OpenError_FailOpenBXCAD,
        OpenError_FailOpenTPL,
        OpenError_MissingCTPK,
        OpenError_FailOpenCTPK,
        OpenError_NoCTPKTextures,
        OpenError_LayoutArchive, // The user selected a layout archive instead of a cellanim archive.

        OutError_FailOpenFile,
        OutError_FailBackupFile,
        OutError_ZlibError,
        OutError_FailTextureExport,

        Error_Unknown = -1
    };

    Error currentError { Error_None };

    std::vector<Session> sessions;

private:
    int currentSessionIndex { -1 };
public:
    // Current session being closed. Used for closing while modified warning.
    int sessionClosingIndex { -1 };

private:
    std::mutex mtx;
};

#endif // SESSIONMANAGER_HPP
