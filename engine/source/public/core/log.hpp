#pragma once

#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace lumina
{
    /**
     * This is a basic class to allow logging to the console using a simple string format
     * The format can be used as follows: `Log::Info("Print variable x: {}", x);`
     *
     * There are currently 4 different method for logging, while functionally the same, the
     * color output is different and used for different levels of importance.
     *
     * Trace is colored grey,  used for logging general information such as what functions happened, so a user might see the execution order of things 
     * Info is colored green,  used for logging information that might contain useful information in the log
     * Warn is colored yellow, used for logging warnings that are non-breaking, but might require the users attention
     * Error is colored red,   used for logging errors that are breaking and require the users attention 
     */
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
