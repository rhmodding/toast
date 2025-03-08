#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <iostream>
#include <fstream>

#include <sstream>
#include <ostream>

#include <iomanip>

#include <string>

#include <mutex>

class Logging {
private:
    Logging() = default;
public:
    ~Logging() = default;

    class LogStream {
    public:
        LogStream(std::ostream& out, const std::string& prefix) :
            outStream(out), logPrefix(prefix)
        {}

        template <typename T>
        LogStream& operator<<(const T& msg) {
            std::lock_guard<std::mutex> lock(mtx);
            buffer << msg;
            return *this;
        }

        LogStream& operator<<(std::ostream& (*manip)(std::ostream&));

    private:
        std::ostream& outStream;
        std::ostringstream buffer;

        std::string logPrefix;

        static std::mutex mtx;

        void flush();

        static std::string getTimestamp();
    };

    static void Open(const std::string& filename);
    static void Close();

    static LogStream info;
    static LogStream warn;
    static LogStream err;

private:
    static std::ofstream logFile;

    static std::mutex mtx;
};

#endif // LOGGING_HPP
