#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include "anim/RvlCellAnim.hpp"

#include "command/BaseCommand.hpp"

#include <cstdint>

#include <mutex>

#include <string>

#include <unordered_map>
#include <deque>

#include <memory>

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

        uint16_t currentCellanim{ 0 };

        ///////////////////////////

        std::string mainPath;

        bool traditionalMethod{ false }; // Has this session been opened with a brcad, png and h file?
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
            return this->sheets.at(this->getCellanimObject()->sheetIndex % this->sheets.size());
        }
        std::unordered_map<uint16_t, std::string>& getAnimationNames() {
            return this->cellanims.at(this->currentCellanim).animNames;
        }

        bool arrangementMode{ false };

        bool modified{ false };

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

    int32_t currentSession{ -1 };

    // Current session being closed. Used for closing while modified warning.
    int32_t sessionClosing{ -1 };

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

    enum SessionError: int16_t {
        SessionError_None = 0,

        SessionOpenError_FailOpenArchive = -1,
        SessionOpenError_FailFindTPL = -2,
        SessionOpenError_RootDirNotFound = -3,
        SessionOpenError_NoBXCADsFound = -4,
        SessionOpenError_FailOpenBXCAD = -5,
        SessionOpenError_FailOpenPNG = -6,
        SessionOpenError_FailOpenHFile = -7,
        SessionOpenError_SessionsFull = -8,

        SessionOutError_FailOpenFile = -9,
        SessionOutError_ZlibError = -10,
        SessionOutError_FailTPLTextureExport = -11,
    } lastSessionError{ SessionError_None };

    // Push session from a Yaz0-compressed Arc file (SZS).
    int32_t PushSessionFromCompressedArc(const char* filePath);
    // Push session from BRCAD, PNG, and H file respectively
    int32_t PushSessionTraditional(const char* paths[3]);

    int32_t ExportSessionCompressedArc(Session* session, const char* outPath);

    void ClearSessionPtr(Session* session);
    void FreeSessionIndex(int32_t index);

    void FreeAllSessions();

private:
    std::mutex mtx;

    SessionManager() {} // Private constructor to prevent instantiation
};

#endif // SESSIONMANAGER_HPP
