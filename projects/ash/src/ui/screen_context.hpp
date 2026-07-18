#pragma once
/// Phase 11: shared context that every Screen reads from.
///
/// One ScreenContext instance is created by App and threaded through
/// every screen push / input event. It carries the live settings, the
/// render palette, and (forward-compat) handles to the player / world /
/// quest engine so screens can render data without owning it.
///
/// Screens may stash typed pointers via `services()` for subsystems
/// that exist in the wider engine but not in v1.
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "render/palette.hpp"
#include "settings/accessibility.hpp"
#include "settings/settings.hpp"

namespace ash {
namespace ui {

/// Typed service registry. Screens register/look up shared subsystems
/// by type. App pre-populates the registry on startup.
class Services {
public:
    template <typename T>
    void set(std::shared_ptr<T> p) {
        map_[std::type_index(typeid(T))] = std::static_pointer_cast<void>(p);
    }

    template <typename T>
    std::shared_ptr<T> get() const {
        auto it = map_.find(std::type_index(typeid(T)));
        if (it == map_.end()) return nullptr;
        return std::static_pointer_cast<T>(it->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> map_;
};

/// Frame timing / debug counters shared with the HUD + debug overlay.
struct FrameStats {
    double last_frame_ms{0.0};
    double avg_frame_ms{0.0};
    int    fps{0};
    int    entity_count{0};
    int    draw_calls{0};
    long   memory_rss_kb{0};
};

/// Live clock / world info shared with HUD.
struct WorldInfo {
    std::string player_name{"Player"};
    std::string location_name{"Unknown"};
    int         current_map_id{0};
    int         time_of_day_minutes{8 * 60};   // 08:00 by default
    int         hp{100};
    int         hp_max{100};
    int         vp{50};
    int         vp_max{50};
    int         sp{30};
    int         sp_max{30};
    int         level{1};
    int         gold{0};
};

/// Top-level context passed to every screen. Holds settings, palette,
/// services, frame stats, and world info. Lives for the app lifetime.
class ScreenContext {
public:
    settings::Settings        settings;
    settings::RenderAccessibility render_acc;
    settings::UiAccessibility    ui_acc;
    render::Palette             palette;
    Services                    services;
    FrameStats                  frame;
    WorldInfo                   world;
    int                         viewport_w{120};
    int                         viewport_h{40};
    bool                        quit_requested{false};

    /// Recompute derived accessibility + palette from `settings`.
    void refresh_accessibility();
};

}  // namespace ui
}  // namespace ash
