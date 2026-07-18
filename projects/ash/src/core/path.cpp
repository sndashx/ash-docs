#include "core/path.hpp"

#include <cstdlib>
#include <mutex>

#if defined(__linux__) || defined(__APPLE__)
#  include <unistd.h>
#elif defined(_WIN32)
#  include <windows.h>
#endif

namespace ash {
namespace core {

namespace {
std::mutex g_path_mu;
std::string g_data_dir_override;
std::string g_config_dir_override;

std::string env_or(const char* var, const std::string& fallback) {
    if (const char* v = std::getenv(var); v && *v) return v;
    return fallback;
}

fs::path home_dir() {
    return env_or("HOME", ".");
}

fs::path xdg_data_home() {
    return env_or("XDG_DATA_HOME", (home_dir() / ".local" / "share").string());
}

fs::path xdg_config_home() {
    return env_or("XDG_CONFIG_HOME", (home_dir() / ".config").string());
}

fs::path exe_dir() {
#if defined(__linux__) || defined(__APPLE__)
    std::error_code ec;
    fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (ec) return {};
    return self.parent_path();
#elif defined(_WIN32)
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return {};
    return fs::path(std::string(buf, n)).parent_path();
#else
    return {};
#endif
}

}  // namespace

void override_data_dir(const std::string& path) {
    std::lock_guard<std::mutex> lk(g_path_mu);
    g_data_dir_override = path;
}

void override_config_dir(const std::string& path) {
    std::lock_guard<std::mutex> lk(g_path_mu);
    g_config_dir_override = path;
}

fs::path data_dir() {
    std::lock_guard<std::mutex> lk(g_path_mu);
    if (!g_data_dir_override.empty()) return g_data_dir_override;
    return xdg_data_home() / "ash";
}

fs::path saves_dir() {
    return data_dir() / "saves";
}

fs::path config_dir() {
    std::lock_guard<std::mutex> lk(g_path_mu);
    if (!g_config_dir_override.empty()) return g_config_dir_override;
    return xdg_config_home() / "ash";
}

fs::path logs_dir() {
    return data_dir() / "logs";
}

fs::path content_dir() {
    std::error_code ec;
    if (const char* v = std::getenv("ASH_CONTENT_DIR"); v && *v) return v;
    fs::path exed = exe_dir();
    if (!exed.empty()) {
        fs::path share = exed / ".." / "share" / "ash" / "content";
        if (fs::is_directory(share, ec)) return fs::canonical(share, ec);
        fs::path here = exed / "content";
        if (fs::is_directory(here, ec)) return fs::canonical(here, ec);
    }
    return "content";
}

fs::path slot_dir(const std::string& savename) {
    return saves_dir() / savename;
}

bool ensure_dir(const fs::path& p) {
    std::error_code ec;
    if (fs::exists(p, ec)) {
        return fs::is_directory(p, ec);
    }
    fs::create_directories(p, ec);
    return fs::is_directory(p, ec);
}

}  // namespace core
}  // namespace ash