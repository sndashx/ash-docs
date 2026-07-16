#include "core/log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace ash
{
namespace log
{
namespace
{
constexpr const char* kLoggerName = "ash";
constexpr const char* kEnvVar = "ASH_LOG_LEVEL";
constexpr const char* kPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";

std::shared_ptr<spdlog::logger>& mutable_logger()
{
    static std::shared_ptr<spdlog::logger> instance =
        []() -> std::shared_ptr<spdlog::logger>
    {
        if (auto existing = spdlog::get(kLoggerName))
        {
            return existing;
        }
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>(kLoggerName, sink);
        logger->set_pattern(kPattern);
        spdlog::register_logger(logger);
        return logger;
    }();
    return instance;
}

std::string to_lower(std::string_view in)
{
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

spdlog::level::level_enum parse_level(std::string_view name)
{
    auto lower = to_lower(name);
    if (lower == "trace")
    {
        return spdlog::level::trace;
    }
    if (lower == "debug")
    {
        return spdlog::level::debug;
    }
    if (lower == "info")
    {
        return spdlog::level::info;
    }
    if (lower == "warn" || lower == "warning")
    {
        return spdlog::level::warn;
    }
    if (lower == "error" || lower == "err")
    {
        return spdlog::level::err;
    }
    if (lower == "critical" || lower == "crit" || lower == "fatal")
    {
        return spdlog::level::critical;
    }
    if (lower == "off" || lower == "none")
    {
        return spdlog::level::off;
    }
    return spdlog::level::info;
}
}  // namespace

void init()
{
    auto& log = mutable_logger();
    const char* raw = std::getenv(kEnvVar);
    auto level = parse_level(raw ? std::string_view(raw) : std::string_view());
    log->set_level(level);
    // Default flush policy: flush on warn and above.
    log->flush_on(spdlog::level::warn);
}

void set_level(spdlog::level::level_enum new_level)
{
    mutable_logger()->set_level(new_level);
}

spdlog::level::level_enum level()
{
    return mutable_logger()->level();
}

void trace(std::string_view message)
{
    mutable_logger()->trace(message);
}
void debug(std::string_view message)
{
    mutable_logger()->debug(message);
}
void info(std::string_view message)
{
    mutable_logger()->info(message);
}
void warn(std::string_view message)
{
    mutable_logger()->warn(message);
}
void error(std::string_view message)
{
    mutable_logger()->error(message);
}
void critical(std::string_view message)
{
    mutable_logger()->critical(message);
}

std::shared_ptr<spdlog::logger> logger()
{
    return mutable_logger();
}
}  // namespace log
}  // namespace ash