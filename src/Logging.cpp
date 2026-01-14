#include "Logging.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/spdlog.h>

namespace sink = spdlog::sinks;

template<typename T, typename... Args>
static std::shared_ptr<spdlog::logger> makeDedupedLogger(std::string name, Args &&...args) {
    auto dedup_sink = std::make_shared<sink::dup_filter_sink_mt>(std::chrono::seconds(10));
    dedup_sink->add_sink(std::make_shared<T>(std::forward<Args>(args)...));
    dedup_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%L%$] %v");
    return std::make_shared<spdlog::logger>(std::move(name), dedup_sink);
}

std::shared_ptr<spdlog::logger> Logging::sStdoutLogger = nullptr;
std::shared_ptr<spdlog::logger> Logging::sStderrLogger = nullptr;
std::shared_ptr<spdlog::logger> Logging::sFileLogger = nullptr;

void Logging::open(std::string_view filename) {
    sStdoutLogger = makeDedupedLogger<sink::stdout_color_sink_mt>("stdout_logger");
    sStderrLogger = makeDedupedLogger<sink::stderr_color_sink_mt>("stderr_logger");

    try {
        sFileLogger = makeDedupedLogger<sink::basic_file_sink_mt>("file_logger", std::string { filename });
    }
    catch (const spdlog::spdlog_ex &ex) {
        sStderrLogger->error("[Logging::open] Failed to open logfile at path \"{}\"!", filename);
        sStderrLogger->error("[Logging::open] {}", ex.what());
    }
}
void Logging::close() {
    sStdoutLogger = nullptr;
    sStderrLogger = nullptr;
    sFileLogger = nullptr;
    spdlog::drop_all();
}
