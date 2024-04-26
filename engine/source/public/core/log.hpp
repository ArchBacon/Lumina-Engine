#pragma once

#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace lumina
{
    class Log
    {
        inline static std::shared_ptr<spdlog::logger> logger;

    public:
        static void Init()
        {
            spdlog::set_pattern("%^[%T] %n: %v%$");
            logger = spdlog::stdout_color_mt("ENGINE");
            logger->set_level(spdlog::level::trace);
        }

        template <typename... Args>
        static void Trace(Args... args)
        {
            logger->trace(std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Info(Args... args)
        {
            logger->info(std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(Args... args)
        {
            logger->warn(std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(Args... args)
        {
            logger->error(std::forward<Args>(args)...);
        }
    };
} // namespace lumina
