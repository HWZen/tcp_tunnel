//
// Created by HWZ on 2023/6/27.
//

#ifndef TCP_TUNNEL_LOGGER_H
#define TCP_TUNNEL_LOGGER_H
#include <spdlog/spdlog.h>
#include <string_view>
#include <source_location>

class Logger : public spdlog::logger {
public:
    Logger(std::string_view name);

};

#define LOG(logger, level, format, ...) \
do{                                     \
    constexpr auto source{std::source_location::current()}; \
    constexpr auto file_path{ std::string_view{source.file_name()} }; \
    constexpr auto file_name{file_path.substr(file_path.find_last_of("\\/") + 1)};\
    logger.level("{}@{}: " format, file_name, source.line(), ##__VA_ARGS__);\
}while(false)                                        \


#define LOG_TRACE(logger, format, ...) LOG(logger, trace, format, ##__VA_ARGS__)
#define LOG_DEBUG(logger, format, ...) LOG(logger, debug, format, ##__VA_ARGS__)
#define LOG_INFO(logger, format, ...) LOG(logger, info, format, ##__VA_ARGS__)
#define LOG_WARN(logger, format, ...) LOG(logger, warn, format, ##__VA_ARGS__)
#define LOG_ERROR(logger, format, ...) LOG(logger, error, format, ##__VA_ARGS__)
#define LOG_CRITICAL(logger, format, ...) LOG(logger, critical, format, ##__VA_ARGS__)

#define TRACE_FUNC(logger) LOG_TRACE(logger, "{}", std::source_location::current().function_name())

#endif //TCP_TUNNEL_LOGGER_H
