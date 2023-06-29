//
// Created by HWZ on 2023/6/27.
//

#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static std::vector<spdlog::sink_ptr>& g_sinks(){
    static std::vector<spdlog::sink_ptr> sinks = [](){
        std::vector<spdlog::sink_ptr> res{};
        auto sink{std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};
        sink->set_level(static_cast<spdlog::level::level_enum>(spdlog::level::trace));
        res.emplace_back(std::move(sink));
        return res;
    }();
    return sinks;
}

Logger::Logger(std::string_view name) : spdlog::logger(std::string{ name }, g_sinks().begin(), g_sinks().end())
{
    set_level(spdlog::level::trace);
}