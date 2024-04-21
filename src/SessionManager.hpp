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
        std::string name;
        bool open{ false };
        uint16_t cellIndex{ 1 };

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

    enum PushSessionError {
        PushSessionError_FailOpenArchive = -1,
        PushSessionError_FailFindTPL = -2,
        PushSessionError_FailFindBXCAD = -3,
        PushSessionError_FailGetTexture = -4,
        PushSessionError_RootDirNotFound = -5,
        PushSessionError_NoBXCADsFound = -6,
    };

    // Push session from Arc file (SZS).
    int16_t PushSessionFromArc(const char* arcPath);
    // Push session from BRCAD, PNG, and H file respectively
    int16_t PushSessionTraditional(const char* paths[3]);

    void FreeSession(Session* session);

    void FreeAllSessions();

private:
    SessionManager() {} // Private constructor to prevent instantiation
};

#endif // SESSIONMANAGER_HPP