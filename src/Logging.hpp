#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <iostream>
#include <fstream>

#include <sstream>
#include <ostream>

#include <string>
#include <string_view>

#include <mutex>

class Logging {
private:
    Logging() = default;
public:
    ~Logging() = default;

    class LogStream {
    public:
        LogStream(std::ostream& out, std::string_view prefix) :
            mOutStream(out), mLogPrefix(prefix)
        {}

        template <typename T>
        LogStream& operator<<(const T& msg) {
            std::lock_guard<std::mutex> lock(mtx);
            mBuffer << msg;
            return *this;
        }

        LogStream& operator<<(std::ostream& (*manip)(std::ostream&));

    private:
        static std::string getTimestamp();

        void flush();

    private:
        static std::mutex mtx;

        std::ostream& mOutStream;
        std::ostringstream mBuffer;

        std::string mLogPrefix;
    };

    static void Open(std::string_view filename);
    static void Close();

    static LogStream info;
    static LogStream warn;
    static LogStream err;

private:
    static std::ofstream logFile;

    static std::mutex mtx;
};

#endif // LOGGING_HPP
