#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "Singleton.hpp"

#include <vector>

#include <mutex>

#include <string_view>

#include "Session.hpp"

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>;

private:
    SessionManager() = default;
public:
    ~SessionManager() = default;

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

public:
    std::vector<Session>& getSessions() { return mSessions; }
    const std::vector<Session>& getSessions() const { return mSessions; }

    Session& getSession(unsigned sessionIndex) {
        if (sessionIndex >= mSessions.size()) {
            throw std::runtime_error(
                "SessionManager::getSession: session index out of bounds (" +
                std::to_string(sessionIndex) + " >= " + std::to_string(mSessions.size()) + ")"
            );
        }
        return mSessions[sessionIndex];
    }
    const Session& getSession(unsigned sessionIndex) const {
        if (sessionIndex >= mSessions.size()) {
            throw std::runtime_error(
                "SessionManager::getSession: session index out of bounds (" +
                std::to_string(sessionIndex) + " >= " + std::to_string(mSessions.size()) + ")"
            );
        }
        return mSessions[sessionIndex];
    }

    unsigned getSessionCount() const { return mSessions.size(); }

    Session* getCurrentSession() {
        if (mCurrentSessionIndex >= 0)
            return &mSessions.at(mCurrentSessionIndex);

        return nullptr;
    }

    bool getCurrentSessionModified() const {
        if (mCurrentSessionIndex >= 0)
            return mSessions.at(mCurrentSessionIndex).modified;
        else
            return false;
    }
    void setCurrentSessionModified(bool modified) {
        if (mCurrentSessionIndex >= 0)
            mSessions.at(mCurrentSessionIndex).modified = modified;
    }

    bool getSessionAvaliable() const {
        return mCurrentSessionIndex >= 0;
    }

    int  getCurrentSessionIndex() const { return mCurrentSessionIndex; }
    void setCurrentSessionIndex(int sessionIndex);

    Error getCurrentError() const { return mCurrentError; }

    int getSessionClosingIndex() const { return mSessionClosingIndex; }
    void setSessionClosingIndex(int index) {
        if (index < 0)
            mSessionClosingIndex = -1;
        else
            mSessionClosingIndex = index;
    }

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

    // Remove all mSessions.
    void RemoveAllSessions();

private:
    std::vector<Session> mSessions;

    Error mCurrentError { Error_None };

    int mCurrentSessionIndex { -1 };
    // Current session being closed. Used for closing while modified warning.
    int mSessionClosingIndex { -1 };

    std::mutex mMtx;
};

#endif // SESSIONMANAGER_HPP
