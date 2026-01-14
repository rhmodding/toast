#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "Singleton.hpp"

#include <cstddef>

#include <vector>

#include <mutex>

#include <string_view>

#include <stdexcept>

#include "Session.hpp"

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>;

private:
    SessionManager() = default;
public:
    ~SessionManager() {
        removeAllSessions();
    }

public:
    std::vector<Session>& getSessions() { return mSessions; }
    const std::vector<Session>& getSessions() const { return mSessions; }
    size_t getSessionCount() const { return mSessions.size(); }

    Session& getSession(size_t index);
    const Session& getSession(size_t index) const;

    // Returns nullptr if no session is selected.
    Session* getCurrentSession();
    const Session* getCurrentSession() const;
    // Returns <0 if no session is selected.
    ssize_t getCurrentSessionIndex() const { return mCurrentSessionIndex; }
    void setCurrentSessionIndex(ssize_t index);

    bool getCurrentSessionModified() const;
    void setCurrentSessionModified(bool modified);

    bool anySessionOpened() const { return mCurrentSessionIndex >= 0; }

    // Create a new session from the path of a cellanim archive (.szs).
    //
    // Returns: index of new session if succeeded, -1 if failed
    ssize_t createSession(std::string_view filePath);
    inline ssize_t createSession(const std::string& filePath) {
        return createSession(std::string_view(filePath));
    }

    // Export a session as a cellanim archive (.szs) to the specified path.
    // Note: if dstFilePath is empty, then the session's resourcePath is used.
    //
    // Returns: true if succeeded, false if failed
    bool exportSession(unsigned sessionIndex, std::string_view dstFilePath = {});

    void removeSession(unsigned sessionIndex);

    void removeAllSessions();

private:
    std::vector<Session> mSessions;
    ssize_t mCurrentSessionIndex { -1 };

    std::mutex mMtx;
};

#endif // SESSION_MANAGER_HPP
