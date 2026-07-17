#include "test_harness.hpp"
#include "editor/editor.hpp"
#include "editor/map.hpp"
#include "editor/tool_fill.hpp"
#include "editor/tool_paint.hpp"
#include "editor/tool_select.hpp"
#include "editor/tool_warp.hpp"
#include "editor/undo.hpp"

#include "render/cell.hpp"

using ash::editor::Editor;
using ash::editor::EntitySpec;
using ash::editor::EntityKind;
using ash::editor::FillCommand;
using ash::editor::IRect;
using ash::editor::IVec2;
using ash::editor::Map;
using ash::editor::MoveEntityCommand;
using ash::editor::PaintCellCommand;
using ash::editor::PaintRectCommand;
using ash::editor::PlaceEntityCommand;
using ash::editor::DeleteEntityCommand;
using ash::editor::Tool;
using ash::editor::UndoStack;
using ash::editor::WarpPlayerCommand;
using ash::render::Cell;

namespace {

Cell make_cell(uint32_t g, std::uint8_t r, std::uint8_t gg, std::uint8_t b) {
    return Cell{ g, r, gg, b };
}

}  // namespace

TEST_CASE("undo: paint cell apply and revert", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    Cell new_c = make_cell(uint32_t('#'), 200, 100, 50);
    u.push(std::make_unique<PaintCellCommand>(1, 3, 2, m.at(1, 3, 2), new_c), m);
    REQUIRE(m.at(1, 3, 2).glyph == uint32_t('#'));
    REQUIRE(u.can_undo());
    REQUIRE(!u.can_redo());
    u.undo(m);
    REQUIRE(m.at(1, 3, 2).glyph == uint32_t(' '));
    REQUIRE(u.can_redo());
    u.redo(m);
    REQUIRE(m.at(1, 3, 2).glyph == uint32_t('#'));
}

TEST_CASE("undo: paint rect stamps and reverts whole region", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    IRect r{ 1, 1, 3, 2 };
    Cell fill = make_cell(uint32_t('.'), 100, 100, 100);
    u.push(std::make_unique<PaintRectCommand>(1, r, fill), m);
    REQUIRE(m.at(1, 1, 1).glyph == uint32_t('.'));
    REQUIRE(m.at(1, 3, 2).glyph == uint32_t('.'));
    REQUIRE(m.at(1, 4, 2).glyph == uint32_t(' '));
    u.undo(m);
    REQUIRE(m.at(1, 1, 1).glyph == uint32_t(' '));
    REQUIRE(m.at(1, 3, 2).glyph == uint32_t(' '));
}

TEST_CASE("undo: fill command records every replaced cell", "[editor][undo]")
{
    Map m(6, 6);
    UndoStack u;
    Cell pre = make_cell(uint32_t('.'), 50, 50, 50);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) m.cell(2, x, y) = pre;
    }
    auto cmd = std::make_unique<FillCommand>(2, 0, 0, make_cell(uint32_t('@'), 0, 0, 0));
    cmd->apply(m);
    REQUIRE(cmd->affected_count() == std::size_t{9});
    REQUIRE(m.at(2, 2, 2).glyph == uint32_t('@'));
    u.push_already_applied(std::move(cmd));
    u.undo(m);
    REQUIRE(m.at(2, 0, 0).glyph == uint32_t('.'));
    REQUIRE(m.at(2, 2, 2).glyph == uint32_t('.'));
}

TEST_CASE("undo: entity place and delete are reversible", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    EntitySpec e;
    e.id = 7;
    e.kind = EntityKind::NPC;
    e.type_id = "npc_a";
    e.pos = { 3, 3 };
    u.push(std::make_unique<PlaceEntityCommand>(e), m);
    REQUIRE(m.entities.size() == std::size_t{1});
    u.undo(m);
    REQUIRE(m.entities.empty());
    u.redo(m);
    REQUIRE(m.entities.size() == std::size_t{1});
    u.push(std::make_unique<DeleteEntityCommand>(7), m);
    REQUIRE(m.entities.empty());
    u.undo(m);
    REQUIRE(m.entities.size() == std::size_t{1});
    REQUIRE(m.entities[0].id == std::uint64_t{7});
}

TEST_CASE("undo: move entity captures and reverts position", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    EntitySpec e;
    e.id = 42;
    e.kind = EntityKind::Item;
    e.type_id = "gold_coin";
    e.pos = { 1, 1 };
    u.push(std::make_unique<PlaceEntityCommand>(e), m);
    u.push(std::make_unique<MoveEntityCommand>(42, IVec2{ 5, 2 }), m);
    REQUIRE(m.entities[0].pos.x == 5);
    REQUIRE(m.entities[0].pos.y == 2);
    u.undo(m);
    REQUIRE(m.entities[0].pos.x == 1);
    REQUIRE(m.entities[0].pos.y == 1);
}

TEST_CASE("undo: warp player captures and reverts position", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    u.push(std::make_unique<WarpPlayerCommand>(IVec2{ 0, 0 }), m);
    REQUIRE(m.entities.size() == std::size_t{1});
    REQUIRE(m.entities[0].id == std::uint64_t{0});
    REQUIRE(m.entities[0].pos.x == 0);
    u.push(std::make_unique<WarpPlayerCommand>(IVec2{ 4, 2 }), m);
    REQUIRE(m.entities[0].pos.x == 4);
    u.undo(m);
    REQUIRE(m.entities[0].pos.x == 0);
}

TEST_CASE("undo: push clears redo stack", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    Cell c = make_cell(uint32_t('#'), 100, 100, 100);
    u.push(std::make_unique<PaintCellCommand>(1, 0, 0, m.at(1, 0, 0), c), m);
    u.undo(m);
    REQUIRE(u.can_redo());
    u.push(std::make_unique<PaintCellCommand>(1, 1, 1, m.at(1, 1, 1), c), m);
    REQUIRE(!u.can_redo());
}

TEST_CASE("undo: max size cap drops oldest", "[editor][undo]")
{
    Map m(8, 4);
    UndoStack u;
    Cell c = make_cell(uint32_t('#'), 100, 100, 100);
    for (int i = 0; i < 1000; ++i) {
        u.push(std::make_unique<PaintCellCommand>(1, i % 8, (i / 8) % 4,
                                                  m.at(1, i % 8, (i / 8) % 4), c), m);
    }
    REQUIRE(u.undo_size() == UndoStack::kMaxSize);
    REQUIRE(UndoStack::kMaxSize == std::size_t{256});
}

TEST_CASE("tool: paint stamps a single rect", "[editor][tool]")
{
    Editor ed;
    ed.active_layer = 1;
    ed.brush_size   = 3;
    ed.paint_cell   = make_cell(uint32_t('@'), 0, 0, 0);
    ash::editor::paint_stamp(ed.map, ed.undo, ed.active_layer, 5, 5,
                             ed.brush_size, ed.paint_cell);
    REQUIRE(ed.map.at(1, 4, 4).glyph == uint32_t('@'));
    REQUIRE(ed.map.at(1, 6, 6).glyph == uint32_t('@'));
    REQUIRE(ed.map.at(1, 7, 7).glyph == uint32_t(' '));
    REQUIRE(ed.undo.undo_size() == std::size_t{1});
    ed.undo.undo(ed.map);
    REQUIRE(ed.map.at(1, 5, 5).glyph == uint32_t(' '));
}

TEST_CASE("tool: fill replaces connected region", "[editor][tool]")
{
    Editor ed;
    ed.active_layer = 1;
    ed.fill_cell = make_cell(uint32_t('#'), 0, 0, 0);
    Cell dot = make_cell(uint32_t('.'), 1, 2, 3);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x) ed.map.cell(1, x, y) = dot;
    ash::editor::fill_at(ed, 0, 0);
    REQUIRE(ed.map.at(1, 1, 1).glyph == uint32_t('#'));
    REQUIRE(ed.map.at(1, 2, 0).glyph == uint32_t(' '));
    ed.undo.undo(ed.map);
    REQUIRE(ed.map.at(1, 1, 1).glyph == uint32_t('.'));
}

TEST_CASE("tool: select delete clears region and is reversible", "[editor][tool]")
{
    Editor ed;
    ed.active_layer = 1;
    Cell c = make_cell(uint32_t('#'), 0, 0, 0);
    for (int i = 0; i < 3; ++i) ed.map.cell(1, i, 0) = c;
    ed.selection = IRect{ 0, 0, 2, 0 };
    ash::editor::select_delete(ed);
    REQUIRE(ed.map.at(1, 0, 0).glyph == uint32_t(' '));
    REQUIRE(ed.map.at(1, 2, 0).glyph == uint32_t(' '));
    while (ed.undo.can_undo()) ed.undo.undo(ed.map);
    REQUIRE(ed.map.at(1, 0, 0).glyph == uint32_t('#'));
    REQUIRE(ed.map.at(1, 2, 0).glyph == uint32_t('#'));
}

TEST_CASE("tool: warp pushes one command per move", "[editor][tool]")
{
    Editor ed;
    ash::editor::warp_to(ed, 5, 5);
    REQUIRE(ed.map.entities.size() == std::size_t{1});
    REQUIRE(ed.map.entities[0].pos.x == 5);
    ash::editor::warp_to(ed, 1, 2);
    REQUIRE(ed.map.entities[0].pos.x == 1);
    REQUIRE(ed.map.entities[0].pos.y == 2);
    ed.undo.undo(ed.map);
    REQUIRE(ed.map.entities[0].pos.x == 5);
}

TEST_CASE("undo: clear empties both stacks", "[editor][undo]")
{
    Map m(4, 4);
    UndoStack u;
    Cell c = make_cell(uint32_t('#'), 0, 0, 0);
    u.push(std::make_unique<PaintCellCommand>(1, 0, 0, m.at(1, 0, 0), c), m);
    u.undo(m);
    REQUIRE(u.can_redo());
    u.clear();
    REQUIRE(!u.can_undo());
    REQUIRE(!u.can_redo());
}