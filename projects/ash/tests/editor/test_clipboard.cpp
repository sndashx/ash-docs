#include "test_harness.hpp"
#include "editor/clipboard.hpp"
#include "editor/editor.hpp"
#include "editor/map.hpp"
#include "editor/undo.hpp"

#include "render/cell.hpp"

using ash::editor::Clipboard;
using ash::editor::Editor;
using ash::editor::EntityKind;
using ash::editor::EntitySpec;
using ash::editor::IRect;
using ash::editor::Map;
using ash::editor::PlaceEntityCommand;
using ash::editor::UndoStack;
using ash::render::Cell;

namespace {

Cell make_cell(uint32_t g, std::uint8_t r, std::uint8_t gg, std::uint8_t b) {
    return Cell{ g, r, gg, b };
}

}  // namespace

TEST_CASE("clipboard: copy snapshot captures cells and entities", "[editor][clipboard]")
{
    Map m(8, 4);
    Cell a = make_cell(uint32_t('A'), 1, 1, 1);
    Cell b = make_cell(uint32_t('B'), 2, 2, 2);
    m.cell(1, 0, 0) = a;
    m.cell(1, 1, 0) = b;
    m.cell(1, 0, 1) = b;
    EntitySpec e;
    e.id = 9;
    e.kind = EntityKind::NPC;
    e.pos = { 0, 0 };
    m.entities.push_back(e);

    Clipboard cb;
    REQUIRE(cb.copy(m, IRect{ 0, 0, 1, 1 }, 1));
    REQUIRE(cb.has_data);
    REQUIRE(cb.layer == 1);
    REQUIRE(cb.region.width() == 2);
    REQUIRE(cb.region.height() == 2);
    REQUIRE(cb.cells.size() == std::size_t{4});
    REQUIRE(cb.entities.size() == std::size_t{1});
}

TEST_CASE("clipboard: paste writes cells into undo stack at new location", "[editor][clipboard]")
{
    Map m(8, 4);
    UndoStack u;
    Cell a = make_cell(uint32_t('A'), 1, 1, 1);
    Cell b = make_cell(uint32_t('B'), 2, 2, 2);
    m.cell(1, 0, 0) = a;
    m.cell(1, 1, 0) = b;

    Clipboard cb;
    REQUIRE(cb.copy(m, IRect{ 0, 0, 1, 0 }, 1));
    std::size_t writes = cb.paste(m, 4, 2, u);
    REQUIRE(writes == std::size_t{2});
    REQUIRE(m.at(1, 4, 2).glyph == uint32_t('A'));
    REQUIRE(m.at(1, 5, 2).glyph == uint32_t('B'));
    // Each write went through undo.
    REQUIRE(u.undo_size() == std::size_t{2});
    u.undo(m);
    REQUIRE(m.at(1, 5, 2).glyph == uint32_t(' '));
}

TEST_CASE("clipboard: paste re-ids entities to avoid duplicates", "[editor][clipboard]")
{
    Map m(8, 4);
    UndoStack u;
    EntitySpec e;
    e.id = 42;
    e.kind = EntityKind::NPC;
    e.type_id = "npc_a";
    e.pos = { 0, 0 };
    m.entities.push_back(e);

    Clipboard cb;
    REQUIRE(cb.copy(m, IRect{ 0, 0, 0, 0 }, 1));
    REQUIRE(cb.entities.size() == std::size_t{1});
    std::size_t writes = cb.paste(m, 3, 3, u);
    REQUIRE(writes == std::size_t{1});
    REQUIRE(m.entities.size() == std::size_t{2});
    // New id != old id, position offset.
    EntitySpec const* pasted = nullptr;
    for (auto const& ent : m.entities) {
        if (ent.id != 42) pasted = &ent;
    }
    REQUIRE(pasted != nullptr);
    REQUIRE(pasted->pos.x == 3);
    REQUIRE(pasted->pos.y == 3);
    REQUIRE(pasted->id != std::uint64_t{42});
}

TEST_CASE("clipboard: cut clears source cells", "[editor][clipboard]")
{
    Map m(8, 4);
    Cell a = make_cell(uint32_t('A'), 1, 1, 1);
    m.cell(1, 0, 0) = a;
    m.cell(1, 1, 0) = a;

    Clipboard cb;
    cb.cut(m, IRect{ 0, 0, 1, 0 }, 1);
    REQUIRE(m.at(1, 0, 0).glyph == uint32_t(' '));
    REQUIRE(m.at(1, 1, 0).glyph == uint32_t(' '));
    REQUIRE(cb.has_data);
    REQUIRE(cb.cells.size() == std::size_t{2});
}

TEST_CASE("clipboard: copy out of bounds fills with CELL_BLANK", "[editor][clipboard]")
{
    Map m(4, 4);
    Cell a = make_cell(uint32_t('A'), 1, 1, 1);
    m.cell(1, 0, 0) = a;
    Clipboard cb;
    REQUIRE(cb.copy(m, IRect{ -2, -2, 1, 1 }, 1));
    REQUIRE(cb.cells.size() == std::size_t{16});
    // Top-left of the snapshot is blank (out of bounds).
    REQUIRE(cb.cells[0].glyph == uint32_t(' '));
    // Cell (2, 2) in the snapshot corresponds to (0, 0) of the map
    // which is 'A'.
    REQUIRE(cb.cells[10].glyph == uint32_t('A'));
}

TEST_CASE("clipboard: invalid layer returns false and does not mutate", "[editor][clipboard]")
{
    Map m(4, 4);
    Clipboard cb;
    REQUIRE(!cb.copy(m, IRect{ 0, 0, 1, 1 }, 0));   // 0 is invalid
    REQUIRE(!cb.copy(m, IRect{ 0, 0, 1, 1 }, 99));  // 99 out of range
    REQUIRE(!cb.has_data);
}

TEST_CASE("clipboard: clear empties the buffer", "[editor][clipboard]")
{
    Map m(4, 4);
    m.cell(1, 0, 0) = make_cell(uint32_t('A'), 1, 1, 1);
    Clipboard cb;
    REQUIRE(cb.copy(m, IRect{ 0, 0, 0, 0 }, 1));
    REQUIRE(cb.has_data);
    cb.clear();
    REQUIRE(!cb.has_data);
    REQUIRE(cb.cells.empty());
    REQUIRE(cb.entities.empty());
}

TEST_CASE("clipboard: paste skipped cells out of destination bounds", "[editor][clipboard]")
{
    Map m(4, 4);
    Cell a = make_cell(uint32_t('A'), 1, 1, 1);
    m.cell(1, 0, 0) = a;
    Clipboard cb;
    REQUIRE(cb.copy(m, IRect{ 0, 0, 0, 0 }, 1));
    UndoStack u;
    // Paste at (3, 3) — only one cell actually writes (the rest are
    // outside the 4x4 map).
    std::size_t writes = cb.paste(m, 3, 3, u);
    REQUIRE(writes == std::size_t{1});
    REQUIRE(m.at(1, 3, 3).glyph == uint32_t('A'));
}