#pragma once
/// Phase 04: Command-pattern undo / redo.
///
/// Each editing operation (paint, fill, place entity, delete entity,
/// move entity, clipboard paste) is wrapped in a `Command` that
/// knows how to `apply` itself to a `Map` and `revert` itself. The
/// `UndoStack` keeps two stacks: an undo stack (commands that can be
/// reverted) and a redo stack (commands that can be re-applied). New
/// commands always clear the redo stack. The undo stack is capped at
/// 256 entries (per the bead's spec) so long edit sessions do not
/// balloon memory.
#include "editor/map.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ash {
namespace editor {

/// Base class for all editor commands. Concrete subclasses override
/// `apply`, `revert`, and `name`. `apply` is invoked once at
/// construction time by `UndoStack::push`; `revert` is invoked from
/// `UndoStack::undo`.
class Command {
public:
    virtual ~Command() = default;
    virtual void        apply(Map& m)    = 0;
    virtual void        revert(Map& m)  = 0;
    virtual std::string name() const    = 0;
};

// --------------------------------------------------------------------
// Paint a single cell on one layer.
// --------------------------------------------------------------------
class PaintCellCommand final : public Command {
public:
    PaintCellCommand(int layer, int x, int y, render::Cell old_c, render::Cell new_c);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "paint_cell"; }

private:
    int layer_;
    int x_, y_;
    render::Cell old_;
    render::Cell new_;
};

// --------------------------------------------------------------------
// Paint a rectangular region on one layer. Captures every prior cell
// in the region so the rectangle can be exactly restored on revert.
// --------------------------------------------------------------------
class PaintRectCommand final : public Command {
public:
    PaintRectCommand(int layer, IRect rect, render::Cell fill);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "paint_rect"; }

    IRect       rect() const noexcept { return rect_; }
    std::size_t affected_count() const noexcept { return old_.size(); }

private:
    int                          layer_;
    IRect                        rect_;
    render::Cell                 fill_;
    std::vector<render::Cell>    old_;     // Size = rect area.
};

// --------------------------------------------------------------------
// Flood-fill from a seed cell. All 4-connected cells matching the
// seed on the same layer are replaced with `replacement_`. The
// affected set is recorded as a list of (x, y, old_cell) so revert
// can restore each cell exactly.
// --------------------------------------------------------------------
class FillCommand final : public Command {
public:
    FillCommand(int layer, int sx, int sy, render::Cell replacement);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "fill"; }

    std::size_t affected_count() const noexcept { return coords_.size(); }

private:
    int                       layer_;
    int                       sx_, sy_;
    render::Cell              replacement_;
    render::Cell              seed_;       // Captured at apply time.
    std::vector<IVec2>        coords_;     // Cells replaced in apply order.
    std::vector<render::Cell> prior_;      // Parallel: prior value at coords_[i].
};

// --------------------------------------------------------------------
// Place an entity. `revert` removes the entity; if the entity was
// later moved/deleted by another command we still remove the
// specific id we placed.
// --------------------------------------------------------------------
class PlaceEntityCommand final : public Command {
public:
    explicit PlaceEntityCommand(EntitySpec spec);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "place_entity"; }

    std::uint64_t entity_id() const noexcept { return spec_.id; }

private:
    EntitySpec spec_;
};

// --------------------------------------------------------------------
// Delete an entity. Captures the spec for restoration on revert.
// --------------------------------------------------------------------
class DeleteEntityCommand final : public Command {
public:
    explicit DeleteEntityCommand(std::uint64_t id);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "delete_entity"; }

    std::uint64_t entity_id() const noexcept { return id_; }

private:
    std::uint64_t id_;
    EntitySpec    saved_;        // Filled in by apply() before removal.
    bool          captured_ = false;
};

// --------------------------------------------------------------------
// Move an entity. Stores the prior and new positions; revert restores
// the original coordinates.
// --------------------------------------------------------------------
class MoveEntityCommand final : public Command {
public:
    MoveEntityCommand(std::uint64_t id, IVec2 to);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "move_entity"; }

private:
    std::uint64_t id_;
    IVec2         from_{};
    IVec2         to_;
};

// --------------------------------------------------------------------
// Warp player position. Editor-only utility: moving the player to a
// clicked cell. Revert restores the original (x, y).
// --------------------------------------------------------------------
class WarpPlayerCommand final : public Command {
public:
    explicit WarpPlayerCommand(IVec2 to);
    void        apply(Map& m) override;
    void        revert(Map& m) override;
    std::string name() const override { return "warp_player"; }

    IVec2 target() const noexcept { return to_; }

private:
    IVec2 from_{};
    IVec2 to_;
};

// --------------------------------------------------------------------
// UndoStack. Cap is 256 per the bead. Each `push` already applies
// the command; callers do not need to invoke apply themselves.
// --------------------------------------------------------------------
class UndoStack {
public:
    /// Cap matches the bead's spec for Phase 4 (256 entries).
    inline static constexpr std::size_t kMaxSize = 256;

    void        push(std::unique_ptr<Command> cmd, Map& m);
    /// Like `push` but the caller has already invoked `apply`.
    /// Required for `FillCommand`, where capturing the affected cell
    /// count must happen between apply and the redo-clear step.
    void        push_already_applied(std::unique_ptr<Command> cmd) noexcept;
    bool        can_undo() const noexcept { return !undo_.empty(); }
    bool        can_redo() const noexcept { return !redo_.empty(); }
    std::size_t undo_size() const noexcept { return undo_.size(); }
    std::size_t redo_size() const noexcept { return redo_.size(); }

    /// Pop the most recent command from the undo stack, revert it,
    /// and push it onto the redo stack. Returns false if undo is empty.
    bool undo(Map& m);
    /// Pop the most recent command from the redo stack, re-apply it,
    /// and push it onto the undo stack. Returns false if redo is empty.
    bool redo(Map& m);

    void clear() noexcept;

private:
    std::vector<std::unique_ptr<Command>> undo_;
    std::vector<std::unique_ptr<Command>> redo_;
};

}  // namespace editor
}  // namespace ash
