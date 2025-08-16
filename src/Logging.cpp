#include "Logging.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger> Logging::sStdoutLogger = nullptr;
std::shared_ptr<spdlog::logger> Logging::sStderrLogger = nullptr;
std::shared_ptr<spdlog::logger> Logging::sFileLogger = nullptr;

void Logging::Open(std::string_view filename) {
    sStdoutLogger = spdlog::stdout_color_mt("stdout_logger");
    sStderrLogger = spdlog::stderr_color_mt("stderr_logger");

    try {
        sFileLogger = spdlog::basic_logger_mt("file_logger", filename.data());
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "[Logging::Open] Failed to open logfile at path \"" << filename << "\"!" << std::endl;
        std::cerr << "[Logging::Open] " << ex.what() << std::endl;
    }
}
void Logging::Close() {
    sStdoutLogger = nullptr;
    sStderrLogger = nullptr;
    sFileLogger = nullptr;
    spdlog::drop_all();
}
