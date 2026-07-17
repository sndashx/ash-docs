#include "editor/undo.hpp"

#include <algorithm>
#include <deque>

namespace ash {
namespace editor {

// ====================================================================
// PaintCellCommand
// ====================================================================
PaintCellCommand::PaintCellCommand(int layer, int x, int y,
                                   render::Cell old_c, render::Cell new_c)
    : layer_(layer), x_(x), y_(y), old_(old_c), new_(new_c) {}

void PaintCellCommand::apply(Map& m) {
    if (!m.in_bounds(x_, y_) || !m.valid_layer(layer_)) return;
    m.cell(layer_, x_, y_) = new_;
}

void PaintCellCommand::revert(Map& m) {
    if (!m.in_bounds(x_, y_) || !m.valid_layer(layer_)) return;
    m.cell(layer_, x_, y_) = old_;
}

// ====================================================================
// PaintRectCommand
// ====================================================================
PaintRectCommand::PaintRectCommand(int layer, IRect rect, render::Cell fill)
    : layer_(layer), rect_(rect.normalize()), fill_(fill) {}

void PaintRectCommand::apply(Map& m) {
    if (!m.valid_layer(layer_)) return;
    if (rect_.empty()) return;
    old_.clear();
    old_.reserve(static_cast<std::size_t>(rect_.area()));
    for (int y = rect_.y0; y <= rect_.y1; ++y) {
        for (int x = rect_.x0; x <= rect_.x1; ++x) {
            if (!m.in_bounds(x, y)) {
                old_.push_back(render::CELL_BLANK);
                continue;
            }
            old_.push_back(m.at(layer_, x, y));
            m.cell(layer_, x, y) = fill_;
        }
    }
}

void PaintRectCommand::revert(Map& m) {
    if (!m.valid_layer(layer_) || old_.empty()) return;
    std::size_t idx = 0;
    for (int y = rect_.y0; y <= rect_.y1; ++y) {
        for (int x = rect_.x0; x <= rect_.x1; ++x) {
            if (!m.in_bounds(x, y)) { ++idx; continue; }
            m.cell(layer_, x, y) = old_[idx++];
        }
    }
}

// ====================================================================
// FillCommand
// ====================================================================
FillCommand::FillCommand(int layer, int sx, int sy, render::Cell replacement)
    : layer_(layer), sx_(sx), sy_(sy), replacement_(replacement) {}

void FillCommand::apply(Map& m) {
    if (!m.valid_layer(layer_) || !m.in_bounds(sx_, sy_)) return;
    seed_   = m.at(layer_, sx_, sy_);
    coords_.clear();
    prior_.clear();

    // 4-connected BFS over same-layer cells whose glyph matches the seed.
    // We use a deque + visited set; the cell layer doesn't change.
    std::vector<char> visited(static_cast<std::size_t>(m.width * m.height), 0);
    auto idx = [&](int x, int y) {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(m.width)
             + static_cast<std::size_t>(x);
    };
    auto matches_seed = [&](int x, int y) {
        render::Cell const& c = m.at(layer_, x, y);
        return c.glyph == seed_.glyph
            && c.flags == seed_.flags
            && c.fg_r == seed_.fg_r
            && c.fg_g == seed_.fg_g
            && c.fg_b == seed_.fg_b
            && c.bg_r == seed_.bg_r
            && c.bg_g == seed_.bg_g
            && c.bg_b == seed_.bg_b;
    };

    std::deque<IVec2> q;
    q.push_back({sx_, sy_});
    visited[idx(sx_, sy_)] = 1;
    static constexpr int dx[4] = {1, -1, 0, 0};
    static constexpr int dy[4] = {0, 0, 1, -1};

    while (!q.empty()) {
        IVec2 cur = q.front(); q.pop_front();
        if (!matches_seed(cur.x, cur.y)) continue;
        coords_.push_back(cur);
        prior_.push_back(m.at(layer_, cur.x, cur.y));
        m.cell(layer_, cur.x, cur.y) = replacement_;
        for (int k = 0; k < 4; ++k) {
            int nx = cur.x + dx[k];
            int ny = cur.y + dy[k];
            if (!m.in_bounds(nx, ny)) continue;
            std::size_t ni = idx(nx, ny);
            if (visited[ni]) continue;
            visited[ni] = 1;
            q.push_back({nx, ny});
        }
    }
}

void FillCommand::revert(Map& m) {
    if (!m.valid_layer(layer_)) return;
    for (std::size_t i = 0; i < coords_.size(); ++i) {
        IVec2 p = coords_[i];
        if (!m.in_bounds(p.x, p.y)) continue;
        m.cell(layer_, p.x, p.y) = prior_[i];
    }
}

// ====================================================================
// PlaceEntityCommand
// ====================================================================
PlaceEntityCommand::PlaceEntityCommand(EntitySpec spec) : spec_(std::move(spec)) {}

void PlaceEntityCommand::apply(Map& m) {
    m.entities.push_back(spec_);
}

void PlaceEntityCommand::revert(Map& m) {
    auto it = std::find_if(m.entities.begin(), m.entities.end(),
        [&](EntitySpec const& e) { return e.id == spec_.id; });
    if (it != m.entities.end()) m.entities.erase(it);
}

// ====================================================================
// DeleteEntityCommand
// ====================================================================
DeleteEntityCommand::DeleteEntityCommand(std::uint64_t id) : id_(id) {}

void DeleteEntityCommand::apply(Map& m) {
    auto it = std::find_if(m.entities.begin(), m.entities.end(),
        [&](EntitySpec const& e) { return e.id == id_; });
    if (it == m.entities.end()) return;
    saved_ = *it;
    captured_ = true;
    m.entities.erase(it);
}

void DeleteEntityCommand::revert(Map& m) {
    if (!captured_) return;
    // Re-insert preserving original order is not required for an
    // editor-only undo; just push back.
    m.entities.push_back(saved_);
}

// ====================================================================
// MoveEntityCommand
// ====================================================================
MoveEntityCommand::MoveEntityCommand(std::uint64_t id, IVec2 to) : id_(id), to_(to) {}

void MoveEntityCommand::apply(Map& m) {
    for (auto& e : m.entities) {
        if (e.id != id_) continue;
        from_ = e.pos;
        e.pos = to_;
        return;
    }
}

void MoveEntityCommand::revert(Map& m) {
    for (auto& e : m.entities) {
        if (e.id != id_) continue;
        e.pos = from_;
        return;
    }
}

// ====================================================================
// WarpPlayerCommand
//
// Player position lives on the map under a reserved sentinel id (0)
// in the entities list. We treat the entities list as the source of
// truth for player position; this avoids dragging a Player struct
// into the editor header.
// ====================================================================
WarpPlayerCommand::WarpPlayerCommand(IVec2 to) : to_(to) {}

void WarpPlayerCommand::apply(Map& m) {
    for (auto& e : m.entities) {
        if (e.id != 0) continue;
        from_ = e.pos;
        e.pos = to_;
        return;
    }
    // No player entity yet — synthesize one with id=0.
    EntitySpec p;
    p.id = 0;
    p.kind = EntityKind::NPC;
    p.type_id = "player";
    p.pos = to_;
    p.layer = 4;
    from_ = to_;
    m.entities.push_back(p);
}

void WarpPlayerCommand::revert(Map& m) {
    for (auto& e : m.entities) {
        if (e.id != 0) continue;
        e.pos = from_;
        return;
    }
}

// ====================================================================
// UndoStack
// ====================================================================
void UndoStack::push(std::unique_ptr<Command> cmd, Map& m) {
    if (!cmd) return;
    cmd->apply(m);
    undo_.push_back(std::move(cmd));
    redo_.clear();
    if (undo_.size() > kMaxSize) {
        // Drop oldest entries so the stack never grows past the cap.
        undo_.erase(undo_.begin(),
                    undo_.begin() + static_cast<std::ptrdiff_t>(undo_.size() - kMaxSize));
    }
}

void UndoStack::push_already_applied(std::unique_ptr<Command> cmd) noexcept {
    if (!cmd) return;
    undo_.push_back(std::move(cmd));
    redo_.clear();
    if (undo_.size() > kMaxSize) {
        undo_.erase(undo_.begin(),
                    undo_.begin() + static_cast<std::ptrdiff_t>(undo_.size() - kMaxSize));
    }
}

bool UndoStack::undo(Map& m) {
    if (undo_.empty()) return false;
    auto cmd = std::move(undo_.back());
    undo_.pop_back();
    cmd->revert(m);
    redo_.push_back(std::move(cmd));
    return true;
}

bool UndoStack::redo(Map& m) {
    if (redo_.empty()) return false;
    auto cmd = std::move(redo_.back());
    redo_.pop_back();
    cmd->apply(m);
    undo_.push_back(std::move(cmd));
    return true;
}

void UndoStack::clear() noexcept {
    undo_.clear();
    redo_.clear();
}

}  // namespace editor
}  // namespace ash