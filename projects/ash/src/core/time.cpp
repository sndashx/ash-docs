#include "core/time.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <thread>

namespace ash {
namespace core {

namespace {
ClockFn g_test_clock = nullptr;

std::int64_t real_unix_seconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

std::int64_t real_unix_millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::int64_t real_monotonic_millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
}  // namespace

std::int64_t unix_seconds() {
    return g_test_clock ? g_test_clock() : real_unix_seconds();
}

std::int64_t unix_millis() {
    return real_unix_millis();
}

std::int64_t monotonic_millis() {
    return real_monotonic_millis();
}

std::string iso8601_utc(std::int64_t u) {
    if (u <= 0) u = real_unix_seconds();
    std::time_t t = static_cast<std::time_t>(u);
    std::tm tm{};
    gmtime_r(&t, &tm);
    char buf[80];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

std::int64_t parse_iso8601(const std::string& s) {
    int y=0, mo=0, d=0, h=0, mi=0, se=0;
    if (std::sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
                    &y, &mo, &d, &h, &mi, &se) < 3) {
        return 0;
    }
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = se;
    return static_cast<std::int64_t>(timegm(&tm));
}

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void set_clock_for_tests(ClockFn fn) {
    g_test_clock = fn;
}

}  // namespace core
}  // namespace ash