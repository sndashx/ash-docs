#pragma once

/// Phase 00: spdlog wrapper used by the rest of the engine.
///
/// Public API:
///   - ash::log::init()           — read ASH_LOG_LEVEL, configure spdlog
///   - ash::log::set_level(level)  — override level at runtime
///   - ash::log::level()           — query current level
///   - ash::log::trace/debug/info/warn/error/critical(message)
///   - ash::log::logger()          — access the underlying spdlog::logger
///
/// The wrapped logger is named "ash" and uses the pattern
/// "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v".
#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

namespace ash
{
namespace log
{
/// Initialize logging. Reads the ASH_LOG_LEVEL environment variable and
/// applies it to the spdlog "ash" logger. Accepts (case-insensitive):
///   trace | debug | info | warn | error | critical | off
/// Defaults to info if unset or unrecognized.
void init();

/// Apply a level to the "ash" logger at runtime.
void set_level(spdlog::level::level_enum level);

/// Current effective level of the "ash" logger.
spdlog::level::level_enum level();

/// Convenience wrappers. Accept either a plain message or fmt-style
/// formatting arguments via the variadic overloads below.
void trace(std::string_view message);
void debug(std::string_view message);
void info(std::string_view message);
void warn(std::string_view message);
void error(std::string_view message);
void critical(std::string_view message);

template <typename... Args>
void trace(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::default_logger_raw()->log(spdlog::level::trace,
        fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void debug(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::default_logger_raw()->log(spdlog::level::debug,
        fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void info(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::default_logger_raw()->log(spdlog::level::info,
        fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void warn(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::default_logger_raw()->log(spdlog::level::warn,
        fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void error(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::default_logger_raw()->log(spdlog::level::err,
        fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void critical(fmt::format_string<Args...> fmt, Args&&... args)
{
    spdlog::default_logger_raw()->log(spdlog::level::critical,
        fmt, std::forward<Args>(args)...);
}

/// Access the underlying spdlog logger named "ash".
std::shared_ptr<spdlog::logger> logger();
}  // namespace log
}  // namespace ash
