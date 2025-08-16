#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <iostream>
#include <fstream>

#include <memory>
#include <sstream>
#include <ostream>

#include <string>
#include <string_view>

#include <mutex>

#include <spdlog/common.h>
#include <spdlog/logger.h>

class Logging {
private:
    Logging() = default;
public:
    ~Logging() = default;

    static void Open(std::string_view filename);
    static void Close();

    template <typename... Args>
    static inline void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (sStdoutLogger != nullptr)
            sStdoutLogger->info(fmt, std::forward<Args>(args)...);

        if (sFileLogger != nullptr)
            sFileLogger->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (sStdoutLogger != nullptr)
            sStdoutLogger->warn(fmt, std::forward<Args>(args)...);

        if (sFileLogger != nullptr)
            sFileLogger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (sStderrLogger != nullptr)
            sStderrLogger->error(fmt, std::forward<Args>(args)...);

        if (sFileLogger != nullptr)
            sFileLogger->error(fmt, std::forward<Args>(args)...);
    }

private:
    static std::shared_ptr<spdlog::logger> sStdoutLogger;
    static std::shared_ptr<spdlog::logger> sStderrLogger;
    static std::shared_ptr<spdlog::logger> sFileLogger;
};

#endif // LOGGING_HPP
