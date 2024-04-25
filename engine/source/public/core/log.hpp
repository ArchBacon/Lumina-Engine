#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace lumina
{
    class Log
    {
    public:
        static void Init();

        inline static std::shared_ptr<spdlog::logger>& GetLogger() { return sLogger; }

    private:
        static std::shared_ptr<spdlog::logger> sLogger;
    };
}

// Logging macros
#define LE_TRACE(...) ::lumina::Log::GetLogger()->trace(__VA_ARGS__)
#define LE_INFO(...)  ::lumina::Log::GetLogger()->info(__VA_ARGS__)
#define LE_WARN(...)  ::lumina::Log::GetLogger()->warn(__VA_ARGS__)
#define LE_ERROR(...) ::lumina::Log::GetLogger()->error(__VA_ARGS__)
#define LE_FATAL(...) ::lumina::Log::GetLogger()->fatal(__VA_ARGS__)
