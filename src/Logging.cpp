#include "Logging.hpp"

#include <chrono>
#include <ctime>

#include <iomanip>

std::ofstream Logging::logFile;
std::mutex Logging::mtx;

std::mutex Logging::LogStream::mtx;

Logging::LogStream Logging::info(std::cout, "[INFO] ");
Logging::LogStream Logging::warn(std::cout, "[WARN] ");
Logging::LogStream Logging::err(std::cerr, "[ERROR] ");

Logging::LogStream& Logging::LogStream::operator<<(
    std::ostream& (*manip)(std::ostream&)
) {
    std::lock_guard<std::mutex> lock(mtx);

    if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
        flush();
    return *this;
}

std::string Logging::LogStream::getTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTm;
#if defined(_WIN32)
    localtime_s(&localTm, &nowTime);
#else
    localtime_r(&nowTime, &localTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTm, "[%Y-%m-%d %H:%M:%S]");

    return oss.str();
}

void Logging::LogStream::flush() {
    const std::string timestamp = getTimestamp();
    const std::string finalMessage = timestamp + " " + logPrefix + buffer.str();

    if (outStream) {
        outStream << finalMessage << '\n';
        outStream.flush();
    }

    if (logFile.is_open()) {
        logFile << finalMessage << '\n';
        logFile.flush();
    }

    buffer.str({});
    buffer.clear();
}

void Logging::Open(std::string_view filename) {
    std::lock_guard<std::mutex> lock(mtx);

    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open())
        std::cerr << "[Logging::Open] Failed to open logfile at path \"" << filename << "\"!" << std::endl;
}
void Logging::Close() {
    std::lock_guard<std::mutex> lock(mtx);

    if (logFile.is_open())
        logFile.close();
}
