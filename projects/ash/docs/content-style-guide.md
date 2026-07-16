# Content Style Guide

## JSON conventions

- One record per file for NPCs, items, factions, creatures, books,
  and quests. Top-level records are objects keyed by stable `id`.
- One JSON array per file for homogeneous collections (bestiary
  categories, item tables, dialogue line lists).
- Lowercase snake_case for all IDs and file names.
- Human-readable display strings live under `display_name` (singular)
  or `name` (proper noun, e.g. faction names).
- All paths are repo-relative, no absolute paths.
- Strings are UTF-8. No smart quotes — straight ASCII or escape.

## CP437 glyph usage

- Tiles use the IBM Code Page 437 box-drawing and block-drawing
  range (``-`ÿ`) for in-game glyphs. Reserve them in your
  editor's character map so you can see what the player will see.
- Avoid mixing box-drawing characters across layers — each layer's
  grid must align cleanly with the others.

## .xp layer conventions

- Maps are REXPaint files using 9 layers. The full layer spec is in
  `appendices/A-rexpaint-layers.md` (see the worktree `appendices/`).
- Layer 1 = terrain, Layer 5 = entities at rest, Layer 9 = post-fx.
- Always populate layers top-down; lower-numbered layers are
  drawn first, higher-numbered layers composite on top.
