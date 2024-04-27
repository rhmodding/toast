#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include <cstdint>

#include "anim/RvlCellAnim.hpp"
#include "common.hpp"

#include <unordered_map>
#include <string>

// Stores instance of SessionManager in local sessionManager.
#define GET_SESSION_MANAGER SessionManager& sessionManager = SessionManager::getInstance()

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>; // Allow access to base class constructor

public:
    struct Session {
        std::string mainPath;
        bool open{ false };
        uint16_t cellIndex{ 0 };

        bool traditionalMethod{ false };
        std::string* pngPath{ nullptr };
        std::string* headerPath{ nullptr };

        std::vector<RvlCellAnim::RvlCellAnimObject*> cellanims;
        std::vector<Common::Image*> cellanimSheets;
        std::vector<std::unordered_map<uint16_t, std::string>*> animationNames;
        std::vector<std::string> cellNames;

        RvlCellAnim::RvlCellAnimObject* getCellanim() {
            return this->cellanims.at(this->cellIndex);
        }
        Common::Image* getCellanimSheet() {
            return this->cellanimSheets.at(this->cellIndex);
        }
        std::unordered_map<uint16_t, std::string>* getAnimationNames() {
            return this->animationNames.at(this->cellIndex);
        }
    } sessions[64];
    int16_t currentSession{ -1 };

    Session* getCurrentSession() {
        if (this->currentSession >= 0)
            return &this->sessions[this->currentSession];
        
        return nullptr;
    }

    void SessionChanged();

    enum SessionOpenError: int16_t {
        SessionOpenError_None = 0,
        SessionOpenError_FailOpenArchive = -1,
        SessionOpenError_FailFindTPL = -2,
        SessionOpenError_RootDirNotFound = -3,
        SessionOpenError_NoBXCADsFound = -4,
        SessionOpenError_FailOpenBXCAD = -5,
        SessionOpenError_FailOpenPNG = -6,
        SessionOpenError_FailOpenHFile = -7,
        SessionOpenError_SessionsFull = -8,
    } lastSessionError{ SessionOpenError_None };

    // Push session from Arc file (SZS).
    int16_t PushSessionFromArc(const char* arcPath);
    // Push session from BRCAD, PNG, and H file respectively
    int16_t PushSessionTraditional(const char* paths[3]);

    void FreeSession(Session* session);

    void FreeAllSessions();

private:
    int16_t firstFreeSessionIndex();

    SessionManager() {} // Private constructor to prevent instantiation
};

#endif // SESSIONMANAGER_HPP