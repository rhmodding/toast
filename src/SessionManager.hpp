#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include "anim/RvlCellAnim.hpp"

#include "command/BaseCommand.hpp"

#include <cstdint>

#include <string>

#include <unordered_map>
#include <deque>

#include <memory>

#include <mutex>

#include "common.hpp"

#define SESSION_MAX_COMMANDS 128

// Stores instance of SessionManager in local sessionManager.
#define GET_SESSION_MANAGER SessionManager& sessionManager = SessionManager::getInstance()

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>; // Allow access to base class constructor

public:
    struct Session {
        struct CellanimData {
            std::shared_ptr<RvlCellAnim::RvlCellAnimObject> object;
            std::string name;
            std::unordered_map<uint16_t, std::string> animNames;
        };
        std::vector<CellanimData> cellanims;
        std::vector<std::shared_ptr<Common::Image>> sheets;

        unsigned currentCellanim { 0 };

        ///////////////////////////

        std::string mainPath;

        bool traditionalMethod { false }; // Has this session been opened with a brcad, png and h file?
        std::unique_ptr<std::string> pngPath;
        std::unique_ptr<std::string> headerPath;

        ///////////////////////////

        std::shared_ptr<RvlCellAnim::RvlCellAnimObject> getCellanimObject() {
            return this->cellanims.at(this->currentCellanim).object;
        }
        std::string& getCellanimName() {
            return this->cellanims.at(this->currentCellanim).name;
        }
        std::shared_ptr<Common::Image>& getCellanimSheet() {
            return this->sheets.at(std::min<unsigned>(this->getCellanimObject()->sheetIndex, this->sheets.size() - 1));
        }
        std::unordered_map<uint16_t, std::string>& getAnimationNames() {
            return this->cellanims.at(this->currentCellanim).animNames;
        }

        bool arrangementMode { false };

        bool modified { false };

    private:
        std::deque<std::shared_ptr<BaseCommand>> undoStack;
        std::deque<std::shared_ptr<BaseCommand>> redoStack;

    public:
        void executeCommand(std::shared_ptr<BaseCommand> command) {
            command->Execute();
            if (undoStack.size() == SESSION_MAX_COMMANDS) {
                undoStack.pop_front();
            }
            undoStack.push_back(command);

            redoStack.clear();
        }

        void undo() {
            if (undoStack.empty())
                return;

            auto command = undoStack.back();
            undoStack.pop_back();

            command->Rollback();

            redoStack.push_back(command);

            this->modified = true;
        }

        void redo() {
            if (redoStack.empty())
                return;

            auto command = redoStack.back();
            redoStack.pop_back();

            command->Execute();

            if (undoStack.size() == SESSION_MAX_COMMANDS)
                undoStack.pop_front();

            undoStack.push_back(command);

            this->modified = true;
        }

        bool canUndo() {
            return !this->undoStack.empty();
        }

        bool canRedo() {
            return !this->redoStack.empty();
        }

        void clearUndoRedoStack() {
            this->undoStack.clear();
            this->redoStack.clear();
        }
    };
    std::deque<Session> sessionList;

    int currentSession { -1 };

    // Current session being closed. Used for closing while modified warning.
    int sessionClosing { -1 };

    Session* getCurrentSession() {
        if (this->currentSession >= 0)
            return &this->sessionList.at(this->currentSession);

        return nullptr;
    }

    bool& getCurrentSessionModified() {
        return this->getCurrentSession()->modified;
    }

    inline bool getSessionAvaliable() {
        return this->currentSession >= 0;
    }

    void SessionChanged();

    enum SessionError {
        SessionError_None = 0,

        SessionOpenError_FailOpenArchive = -0xFF,
        SessionOpenError_FailFindTPL,
        SessionOpenError_RootDirNotFound,
        SessionOpenError_NoBXCADsFound,
        SessionOpenError_FailOpenBXCAD,
        SessionOpenError_FailOpenTPL,
        SessionOpenError_FailOpenPNG,
        SessionOpenError_FailOpenHFile,
        SessionOpenError_SessionsFull,

        SessionOutError_FailOpenFile,
        SessionOutError_ZlibError,
        SessionOutError_FailTPLTextureExport,
    } lastSessionError{ SessionError_None };

    // Push session from a Yaz0-compressed U8 archive (SZS).
    int PushSessionFromCompressedArc(const char* filePath);
    // Push session from BRCAD, PNG, and H file respectively
    int PushSessionTraditional(const char* paths[3]);

    int ExportSessionCompressedArc(Session* session, const char* outPath);

    void ClearSessionPtr(Session* session);
    void FreeSessionIndex(int index);

    void FreeAllSessions();

private:
    std::mutex mtx;

    SessionManager() {} // Private constructor to prevent instantiation
};

#endif // SESSIONMANAGER_HPP
