# Architecture

ASH is organized around 9 system pillars (full spec in
`04-pillars.txt` at the worktree root).

1. **World Data Model** — Maps are 80x24 cells in 9 REXPaint layers.
   Entities live in an entt registry scoped per-map.
2. **REXPaint Pipeline** — `.xp` files (column-major, gzip-wrapped
   binary) are the canonical format for all spatial data; layers 1-9
   have well-defined meanings.
3. **Terminal Renderer** — A layered compositor paints front/back
   buffers with truecolor ANSI escapes. Camera, light grid, palette,
   and glyph animation are first-class.
4. **Editor Mode** — In-game brush/select/pick/fill/entity/warp tools
   with command-pattern undo and clipboard.
5. **Input Layer** — Key + mouse parsing with a stack of input modes
   (game, menu, dialogue, inventory, editor) and a JSON keybind table.
6. **Character System** — 9 attributes, 24 skills, XP curves, derived
   stats (HP/VP/SP), inventory + equipment, status conditions.
7. **Combat** — Round manager, attack resolution, damage + resistance,
   pathfinding, line-of-sight, cover, AI behaviors.
8. **NPC / Dialogue / Quest** — NPC factory + schedules + disposition,
   a hand-rolled dialogue scripting language (`dlg_lexer`, `dlg_parser`,
   `dlg_eval`), and a stage-machine quest engine with consequence
   rules and an auto-generated journal.
9. **Persistence + Polish** — Save/load with a versioned schema
   (`schema_v0`, `migrate`), autosave tick, accessibility settings,
   and content hot-reload for development.
