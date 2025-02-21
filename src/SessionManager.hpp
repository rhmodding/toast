#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include "texture/Texture.hpp"

#include "texture/TextureGroup.hpp"

#include "anim/RvlCellAnim.hpp"

#include "command/BaseCommand.hpp"

#include <cstdint>

#include <string>

#include <unordered_map>
#include <deque>

#include <memory>

#include <mutex>

#include "common.hpp"

class Session {
public:
    Session() = default;
    ~Session() = default;

    Session(Session&& other) noexcept
        : cellanims(std::move(other.cellanims)),
          sheets(std::move(other.sheets)),
          currentCellanim(other.currentCellanim),
          resourcePath(std::move(other.resourcePath)),
          arrangementMode(other.arrangementMode),
          modified(other.modified),
          undoQueue(std::move(other.undoQueue)),
          redoQueue(std::move(other.redoQueue)) {}

    Session& operator=(Session&& other) noexcept {
        if (this != &other) {
            cellanims = std::move(other.cellanims);
            sheets = std::move(other.sheets);
            currentCellanim = other.currentCellanim;
            resourcePath = std::move(other.resourcePath);
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
