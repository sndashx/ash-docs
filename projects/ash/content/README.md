# Content

ASH is JSON- and .xp-driven. Game data lives in two parallel tracks:

**JSON** for structured records — regions, NPCs, items, factions,
bestiary, schedules, dialogue scripts, and quest files. JSON keeps the
data diff-friendly and lets us hand-author content in a normal editor.

**REXPaint .xp** for spatial data — every in-game map is a multi-layer
REXPaint file (1-9 layers, 80x24 cells, column-major, gzip-wrapped).
Layers follow the convention in `04-pillars.txt` Pillar 2 and the
detailed spec in `appendices/A-rexpaint-layers.md` (see the worktree
`appendices/` dir).

Add new content under the appropriate subdirectory. File names are
lowercase snake_case and mirror the in-game ID.
