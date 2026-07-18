#pragma once
/// Phase 10: game clock + ISO 8601 helpers.
#include <cstdint>
#include <string>

namespace ash {
namespace core {

/// Wall-clock seconds since the Unix epoch (UTC).
std::int64_t unix_seconds();

/// Wall-clock milliseconds since the Unix epoch.
std::int64_t unix_millis();

/// Monotonic milliseconds since some unspecified epoch (steady_clock).
std::int64_t monotonic_millis();

/// ISO 8601 UTC timestamp like "2026-07-15T14:32:11Z".
std::string iso8601_utc(std::int64_t unix_seconds);

/// Parse "YYYY-MM-DDTHH:MM:SSZ" (lenient: missing Z is OK).
/// Returns 0 on parse failure.
std::int64_t parse_iso8601(const std::string& s);

/// Sleep for `ms` milliseconds. Used by the autosave tick to yield.
void sleep_ms(int ms);

/// Inject a clock for tests. Pass nullptr to restore the real clock.
using ClockFn = std::int64_t (*)();
void set_clock_for_tests(ClockFn fn);

}  // namespace core
}  // namespace ash