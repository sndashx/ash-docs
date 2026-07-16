# Ash Sonic v3 — Prodigy GameForge Alignment

> Extends Ash Sonic (the hypersonic coding agent for
> `/home/command/projects/ash`) into the **Prodigy GameForge** shape:
> a doctoral-level, multi-agent game-making system with a persistent
> Game Brain. Pragmatic for a C++20 ASCII RPG — every agent is a
> specialized invocation of the same substrate, not a separate service.

## 1. Mapping Prodigy → Ash Sonic

| Prodigy Layer            | Ash Sonic role                                            | Agent name           | Tools                                    |
|--------------------------|-----------------------------------------------------------|----------------------|------------------------------------------|
| **Producer**             | Decompose intent → phases/milestones, allocate work, track risks | `ash-producer`       | `ash-mem`, plan files, project graph     |
| **Narrative & World**    | Factions, lore, quests, dialogue (`.dlg`, `.qst`, region content) | `ash-narrative`      | `ash-mem`, `dialogue/`, `quest/`, `content/<region>/` |
| **Mechanics & Systems**  | Combat math, skills, status, balance curves                | `ash-mechanics`      | `character/`, `combat/`, balance specs   |
| **Level & Environment**  | Map layout, REXPaint layouts, spawn points, sightlines     | `ash-level`          | `world/`, `content/<region>/`, `.xp`     |
| **Code (Engineer)**      | C++20 impl, entt systems, tests                            | `ash-engineer`       | `src/**`, `tests/**`, `cmake -B build`   |
| **Art & Asset**          | REXPaint assets, palette, typography                       | `ash-art`            | REXPaint (the MCP under `rexpaint:*`)    |
| **Audio & Music**        | Adaptive music layers, SFX (deferred — out of scope for v3)| —                    | —                                        |
| **QA / Playtest Swarm**  | Catch2 tests, manual playthrough scripts, determinism checks | `ash-qa`             | `ctest`, repro scripts, `ep-write`       |
| **Balance & Analytics**  | Damage curves, XP tables, telemetry-style sims             | `ash-balance`        | `combat/test_*`, mock telemetry          |
| **Quality & Polish**     | Code style, formatting, doc updates                        | `ash-polish`         | `clang-format`, `docs/`                  |
| **Game Brain**           | Persistent memory (4 tiers, hybrid retrieval)             | every agent shares `ash-mem` | see memory-design.md              |

Every agent is a *role* the human or another agent invokes; the
underlying substrate is the same: `.ash/memory/` + the project graph.

## 2. The Producer's project graph (concrete schema)

The Game Brain already has a graph; the Producer extends it with
game-specific node kinds *and* extends edges to express:

### New node kinds

- `region`     — Scoured Coast, Glasswood, Riven Spire, Ash Plains,
                 Hollow Cities (one node per region).
- `quest`      — a `.qst` file or in-graph quest.
- `npc`        — named NPC with disposition.
- `dialogue`   — a `.dlg` node.
- `mechanic`   — atomic gameplay rule (e.g. "cover reduces hit_chance
                 by 30%").
- `asset`      — a `.xp` layer or palette entry.
- `milestone`  — Producer-decomposed work item with owner + status.
- `risk`       — known unknown, with mitigation.

### New edge kinds

- `region --hosts-->` `map`
- `map --contains-->` `spawn_group`
- `quest --gated_by-->` `milestone`
- `milestone --owned_by-->` `agent`
- `mechanic --depends_on-->` `symbol`
- `asset --used_in-->` `region`
- `mechanic --breaks-->` `mechanic`           (conflict)
- `decision --supersedes-->` `decision`         (precedence)

Edges of `--kind breaks--` light up as conflicts in `recall`.

## 3. Producer loop (per request)

```
1. RECALL    ash-mem recall "<request>" --k 16 --hops 3
             pull recent decisions, conflicts, owned milestones.

2. DECOMPOSE add `milestone:*` nodes (one per subsystem in the
             request), with --attrs '{"status":"planned","owner":"<role>"}'.
             Link them: milestone A precedes B when B depends on A.

3. ALLOCATE   edge: milestone --owned_by--> agent:<role>
             Use one of: ash-narrative, ash-mechanics, ash-level,
             ash-engineer, ash-art, ash-qa, ash-balance, ash-polish.

4. DISPATCH   for each milestone, dispatch a sub-agent
             (delegate or task) with the exact node id + refs.

5. MONITOR    as agents return, ash-mem ep-write --kind milestone
             --outcome ok|warn|err --refs ["milestone:...", ...].

6. RECONCILE  when all done, Producer updates the graph:
             milestone --status--> done; spawn-risk nodes for any
             open questions; flush working.md.
```

## 4. Per-agent responsibilities

### ash-narrative
- Reads/writes `content/<region>/lore.md`, `.dlg`, `.qst`.
- Update semantics: never breaks an existing `dec:*` edge.
- Refuses to mutate `core/` or `combat/` (delegates to ash-mechanics
  or ash-engineer).

### ash-mechanics
- Owns `character/`, `combat/`, `npc/balance`.
- Causal logic model: every mechanic node links via
  `mechanic --influences--> mechanic` to its cause/effect.
- Validates: a new mechanic must declare at least one
  `mechanic --tested_by--> test:*` node, else refused.

### ash-level
- Owns `world/`, `content/<region>/*.xp`, spawn placements.
- Runs constraint check: every map node has a `map --has_layer--> layer`
  edge for each of the 9 expected layers.

### ash-engineer
- The hypersonic edit loop from v1, plus honoring the milestone it owns.
- Records `dec:*` for any public-API change, links it to the milestone.

### ash-art
- Drives REXPaint (the MCP) to author/edit `.xp` files.
- Maintains the `palette:*` node and links all assets to it.

### ash-qa
- Runs `ctest`, files `err:*` nodes for failures, links to the
  symbol they touch.
- Every red test increases `errors_seen.count` (procedural).

### ash-balance
- Reads `combat::DamageCurve`, `character::XPCurve`; proposes
  parameter tweaks. Refuses to edit code itself — writes
  `mechanic --tune_to--> balance_target` and routes to ash-engineer.

### ash-polish
- Style/format/audit. Adds/checks `.clang-tidy`, doc updates.
- Final pre-release gate: reads every `milestone --status open`
  and the project graph's `--breaks` edges.

## 5. Self-evolving agents (doctoral piece, v3.x target)

- Each agent maintains a `reflexes` table. Reflexes whose
  `success_rate` drops below 0.5 over 20 firings are auto-disabled
  and surfaced as a `risk:*` node.
- New reflexes above 0.8 success-rate over 50 firings are
  auto-promoted to `rulings` (confidence bump).
- This is `agent.sqlite → reflexes` + a small `bin/ash-evolve` cron.

## 6. Co-generation of levels + agents (v3.x target)

- `ash-level` writes a map; immediately `ash-qa` runs a
  deterministic playthrough sim against `agent:player_v1` (a
  scripted `xoshiro256`-seeded agent), records `ep:sim:*`.
- Results feedback into `region --playtest_metric-->` metric:*.
- If metric drops below threshold, `risk:*` node and message
  Producer to re-plan.

## 7. Persistent Game Brain — guarantees

- **Provenance**: every `milestone --done_by--> agent` and every
  `mechanic --tuned_in-->` writes a `provenance` row to `agent.sqlite`.
- **Versioning**: `episodes.session_id` is an incrementing run id; a
  full graph snapshot is dumped to `.ash/memory/snapshots/<id>.json` on
  milestone close.
- **Cross-project patterns**: a separate `~/.ash-memory/patterns/`
  (out of scope for v3) will distill successful mechanic +
  level + quest triples into reusable templates.

## 8. Implementation status in this scaffold

| Layer              | Status        | Files                                              |
|--------------------|---------------|----------------------------------------------------|
| Substrate          | done (v2)     | `.ash/memory/`                                     |
| Memory CLI         | done (v2)     | `bin/ash-mem`                                      |
| Project-graph kinds| in progress   | `bin/ash-graph-seed`                               |
| Per-role entrypoints| in progress  | `bin/ash-producer`, `ash-narrative`, …             |
| Self-evolution     | deferred v3.1 | `bin/ash-evolve` (schema present, cron pending)    |
| RL playtest sim    | deferred v3.1 | scripted `xoshiro256` agent stub                   |
| Cross-project patterns | deferred  | `~/.ash-memory/patterns/`                          |

## 9. Human-in-the-loop gates (unchanged from v1)

- RNG, save schema, `core/` public API: Producer MUST stop & ask.
- New dependency in `CMakeLists.txt`: Producer MUST stop & ask.
- Multiple phases in one milestone: Producer MUST stop & ask.
- Adding a new C++ standard: Producer MUST refuse.

These gates live as `rulings` rows in `agent.sqlite`, inherited
across agents.
