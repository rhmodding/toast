#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include <vector>

#include <mutex>

#include <string_view>

#include "Session.hpp"

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
    int CreateSession(std::string_view filePath);

    // Export a session as a cellanim archive (.szs) to the specified path.
    // Note: if dstFilePath is empty, then the session's resourcePath is used.
    //
    // Returns: true if succeeded, false if failed
    bool ExportSession(unsigned sessionIndex, std::string_view dstFilePath = {});

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
        OpenError_MissingCTPK, // The number of CTPKs is not equal to the number of BCCADs.
        OpenError_FailOpenCTPK,
        OpenError_NoCTPKTextures, // A CTPK exists but contains no textures.
        OpenError_LayoutArchive, // The user selected a layout archive instead of a cellanim archive.
        OpenError_EffectResource, // The user selected an effect file instead of a cellanim archive.
        OpenError_BCRES, // The user selected a BCRES instead of a cellanim archive.

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
