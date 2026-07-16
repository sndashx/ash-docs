# Ash of the Shattered Covenant — Master Plan

> The complete design document for a hand-authored ASCII RPG in C++20 with
> REXPaint-native assets, truecolor terminal rendering, and 200 years of
> broken sky.

This repo is the **plan only** — the artifact a studio would commission
before greenlighting production. The C++ engine, content, and tooling live
elsewhere. See `/home/command/projects/ash` for the live implementation
workspace.

## Reading order

| #  | File | What it is |
|----|------|------------|
| 00 | [conventions](00-conventions.txt)        | File format, step IDs, glossary, naming |
| 01 | [vision](01-vision.txt)                  | One-paragraph pitch + reference games (Morrowind / Fallout 1-2 / BG3 / Dark Souls / Disco Elysium / Planescape) |
| 02 | [decisions](02-decisions.txt)            | 70 architectural decisions (tech stack, patterns, constraints) |
| 03 | [directory](03-directory.txt)            | Full project tree |
| 04 | [pillars](04-pillars.txt)                | 9 core system pillars overview |
|    | [index](index.txt)                       | One-screen index of all 29 docs |

### Engine phases (00-11)

| #  | File | Phase |
|----|------|-------|
| 05 | [phase 00](05-phase-00-skeleton.txt)    | Skeleton (CMake, CPM, deps) |
| 06 | [phase 01](06-phase-01-renderer.txt)    | Renderer (Cell, Buffer, ANSI, Camera, Light) |
| 07 | [phase 02](07-phase-02-world.txt)       | World (entt components, Map, World) |
| 08 | [phase 03](08-phase-03-rexpaint.txt)    | REXPaint pipeline (.xp format, layer system) |
| 09 | [phase 04](09-phase-04-editor.txt)      | Editor (paint, undo, layers, save) |
| 10 | [phase 05](10-phase-05-input.txt)       | Input (UiMode, ModeStack, keybinds, camera) |
| 11 | [phase 06](11-phase-06-character.txt)   | Character (9 attrs, 24 skills, inventory, equipment) |
| 12 | [phase 07](12-phase-07-combat.txt)      | Combat (RTwP, weapons, armor, A*, LoS, AI) |
| 13 | [phase 08](13-phase-08-npc.txt)         | NPC life (schedules, .dlg DSL, dialogue UI, factions) |
| 14 | [phase 09](14-phase-09-quests.txt)       | Quests (.qst DSL, journal, objectives, branches) |
| 15 | [phase 10](15-phase-10-save.txt)        | Save/load (atomic, 3-gen backup, migrations) |
| 16 | [phase 11](16-phase-11-polish.txt)      | Polish (settings, accessibility, build) |

### Content phases (12-23)

| #  | File | Phase |
|----|------|-------|
| 17 | [phase 12](17-phase-12-scoured-coast.txt)  | Scoured Coast (12 maps, ~80 NPCs) |
| 18 | [phase 13](18-phase-13-glasswood.txt)      | Glasswood (20 maps, ~120 NPCs) |
| 19 | [phase 14](19-phase-14-riven-spire.txt)    | Riven Spire (15 maps, ~90 NPCs) |
| 20 | [phase 15](20-phase-15-ash-plains.txt)     | Ash Plains (18 maps, ~80 NPCs) |
| 21 | [phase 16](21-phase-16-hollow-cities.txt)  | Hollow Cities (15 maps, ~100 NPCs) |
| 22 | [phase 17](22-phase-17-main-quest.txt)     | Main quest (5 acts, ~40 stages, ~10 endings) |
| 23 | [phase 18](23-phase-18-factions.txt)       | Faction questlines (4 factions, 7 ranks, 60 quests) |
| 24 | [phase 19](24-phase-19-side-quests.txt)    | Side quests (80 total) |
| 25 | [phase 20](25-phase-20-books.txt)          | Books (62 books, ~95K words) |
| 26 | [phase 21](26-phase-21-items.txt)          | Item catalog (~400 items) |
| 27 | [phase 22](27-phase-22-bestiary.txt)       | Bestiary (~62 creatures) |
| 28 | [phase 23](28-phase-23-release.txt)        | Release (build, distribute, post-launch) |

### Appendices

| #  | File | Reference for |
|----|------|---------------|
| A | [REXPaint layers](appendices/A-rexpaint-layer-spec.txt)  | 9 layers, item glyphs, faction colors |
| B | [.dlg DSL grammar](appendices/B-dialogue-grammar.txt)     | Dialogue trees |
| C | [.qst DSL format](appendices/C-quest-format.txt)         | Quest files |
| D | [Save schema v0](appendices/D-save-schema.txt)           | Save layout, migrations |
| E | [Keybind defaults](appendices/E-keybind-defaults.txt)    | All modes |
| F | [Animation format](appendices/F-animation-format.txt)    | `.json` animation spec |

### Workspace planning

These are working notes for the agent harness that builds the engine.
They're not part of the canonical game spec.

- [agent-design.md](workspace/agent-design.md) — Ash Sonic coding-agent design
- [memory-design.md](workspace/memory-design.md) — 4-tier memory substrate
- [mcp-research.md](workspace/mcp-research.md) — MCP servers + plugins for Ash
- [prodigy-alignment.md](workspace/prodigy-alignment.md) — Multi-agent alignment

## Scope at a glance

- 24 phases (12 engine + 12 content)
- 80 maps across 5 regions
- ~470 named NPCs with daily schedules
- ~400 items, ~62 creatures, ~62 books (~95,000 words prose)
- 4 factions × 7 ranks = 28 progression tracks
- 5-act main quest with ~20 branches and ~10 endings
- 60 faction quests + 80 side quests
- ~6,000 dev hours target (solo dev / small team: 2-5 years)

## License

The plan documents themselves are MIT — share, fork, adapt. The game
implementations (engine code, content assets) retain their own licenses
on the parent project.