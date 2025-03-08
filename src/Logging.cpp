#include "Logging.hpp"

#include <chrono>
#include <ctime>

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
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm {};

#ifdef _WIN32
    localtime_s(&local_tm, &nowTime);
#else
    localtime_r(&nowTime, &local_tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "[%Y-%m-%d %H:%M:%S]");

    return oss.str();
}

void Logging::LogStream::flush() {
    std::string timestamp = getTimestamp();

    std::string finalMessage = timestamp + " " + logPrefix + buffer.str();
    outStream << finalMessage << std::endl;
    outStream.flush();

    if (logFile.is_open()) {
        logFile << finalMessage << std::endl;
        logFile.flush();
    }

    // Clear buffer.
    buffer.str("");
    buffer.clear();
}

void Logging::Open(const std::string& filename) {
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