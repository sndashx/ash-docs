# Ash Sonic v2 — Memory Architecture

> Beyond context-window-only memory. Hybrid (vector + graph) retrieval
> over a four-tier memory model, all stored per-project at
> `/home/command/projects/ash/.ash/memory/`.

## 1. The four tiers

| Tier            | What it holds                                              | Where                                     | Lifetime          |
|-----------------|------------------------------------------------------------|-------------------------------------------|-------------------|
| **Working**     | Current task state, open questions, scratchpad             | in-session + flushed to `working.md`     | Session / N hours |
| **Episodic**    | What happened: turns, edits, builds, errors, decisions     | `memory/episodes` table + vector index    | Forever (TTL'd)   |
| **Semantic**    | Facts about the world: symbols, phases, conventions, KG    | `memory/graph.sqlite` + vectors           | Forever           |
| **Procedural**  | How the agent behaves: preferences, rulings, reflexes      | `memory/agent.sqlite` (per-agent rows)    | Forever, evolves  |

All four are project-scoped to `.ash/memory/` (gitignored) and structured
so they survive `compress`, session restarts, and multi-agent runs.

## 2. Storage layout

```
.ash/memory/
├── graph.sqlite          # Knowledge graph: entities + edges (FTS5 for text)
├── vectors.sqlite        # sqlite-vec ANN index of all episodic + semantic chunks
├── agent.sqlite          # Per-agent procedural memory (preferences + rulings)
├── working.md            # Latest working-snapshot, rewritten each session
├── open-questions.md     # Living list of things to confirm with the human
└── README.md             # This file's short form (one-screen summary)
```

Everything is SQLite + flat files. **No new runtimes, no new services.**
The whole substrate is one `sqlite3` plus `sqlite-vec` (a loadable extension).

## 3. Knowledge graph schema (graph.sqlite)

```
nodes
  id            TEXT PRIMARY KEY         -- e.g. "phase:07", "sym:weapons", "err:asn-2026-07-16-001"
  kind          TEXT NOT NULL INDEXED    -- phase | symbol | file | error | decision | concept | agent
  name          TEXT NOT NULL            -- display name
  attrs         TEXT (JSON)              -- kind-specific metadata
  created_at    INTEGER
  updated_at    INTEGER

edges
  src           TEXT REFERENCES nodes(id)
  dst           TEXT REFERENCES nodes(id)
  kind          TEXT NOT NULL INDEXED    -- depends_on | precedes | fixes | supersedes |
                                          -- belongs_to | implements | conflicts_with |
                                          -- tested_by | mentions | authored_by | decided_in
  weight        REAL DEFAULT 1.0
  attrs         TEXT (JSON)
  created_at    INTEGER
  PRIMARY KEY (src, dst, kind)

nodes_fts                     -- FTS5 index on name + attrs for exact lookup
```

Example edges for Ash:

- `phase:07` ──precedes──▶ `phase:08`  (phase ordering, master plan)
- `sym:combat::attack` ──depends_on──▶ `sym:core::fixed`
- `err:ubsan-null-deref-2026-07-15` ──fixes──▶ `sym:world::spawn_creature`
- `dec:no-cpp23` ──applies_to──▶ `phase:*`
- `agent:ash-sonic` ──authored──▶ `sym:combat::weapons`

## 4. Vector index (vectors.sqlite via sqlite-vec)

```
chunks
  id            TEXT PRIMARY KEY         -- mirrors node id or "ep:<sha256>"
  source_kind   TEXT INDEXED             -- node | episode | doc | turn
  source_id     TEXT INDEXED             -- back-ref to node id or episode id
  text          TEXT NOT NULL
  embedding     vec_f32(N)               -- N = embedding dim (1536 default)
  created_at    INTEGER
vec_chunks_idx USING HNSW(...)           -- ANN
vec_chunk_meta_idx(source_kind, source_id)
```

Embeddings come from a pluggable provider. Default: a local sentence-transformer
via `pip install sentence-transformers`. Embed-once-on-write; cached.

## 5. Episodic store (also vectors.sqlite)

```
episodes
  id            TEXT PRIMARY KEY         -- sha256 of (timestamp + turn text)
  session_id    TEXT INDEXED
  agent         TEXT INDEXED             -- e.g. "ash-sonic" or "human"
  ts            INTEGER
  kind          TEXT INDEXED             -- turn | edit | build | test | error | decision
  summary       TEXT                     -- 1–2 sentences
  payload       TEXT (JSON)              -- full event data
  refs          TEXT (JSON list)         -- node ids touched
  outcome       TEXT INDEXED             -- ok | warn | err | skipped
```

Every tool call the agent makes produces an episode. Episodes are the
agent's autobiographical memory; the agent can recall any past turn.

## 6. Procedural memory (agent.sqlite)

Per-agent records, where `agent` is the agent name (`ash-sonic`,
`planner`, `reviewer`, `human`, …):

```
preferences       (agent, key, value, weight, updated_at)
rulings           (agent, sig, rule, evidence_node_id, confidence, ts)
                  -- sig = stable hash of the situation description
reflexes          (agent, trigger_pattern, action, learned_at, hits, success_rate)
errors_seen       (agent, signature, count, last_ts, fix_node_id)
provenance        (agent, decision_node_id, ts, rationale)
```

`preferences` accumulates user preferences ("be concise", "batch tool
calls", "no heredoc"). `rulings` captures every "stop & ask" decision
with evidence so the agent doesn't relearn it.

## 7. Retrieval: hybrid vector + graph

A single router `ash-mem recall "<query>"` runs:

1. **Vector recall** — embed the query, ANN over `vec_chunks_idx`,
   top-K = 12. Return episodic + semantic chunks with similarity scores.
2. **Graph expansion** — for each hit, pull 1-hop and 2-hop neighbors
   from `graph.sqlite`, ranked by edge weight + recency.
3. **Rerank** — merge, dedupe by node id, sort by combined score:
   `0.65 * vector_sim + 0.35 * graph_pagerank`.
4. **Format** — output one block per result:
   `[node-id] (sim 0.81, hops 1) <excerpt>`.

This is the HippoRAG-style pattern: vectors find entry points,
graph follows structure. No LLM in the loop until synthesis.

## 8. What the agent captures automatically

The `ash-sonic` skill (updated for v2) instructs the agent to:

- **Every turn** — write one `episode` row (turn summary + refs).
- **On edit** — record nodes touched, outcome (build/test pass/fail).
- **On error** — record `err:<sig>` node, link to symbol node it touched.
- **On decision** — record `dec:<topic>` node with rationale + evidence.
- **On preference cue** — write to `preferences` with weight 1.0, increment
  weight on repeat matches (confidence grows over time).
- **On every recall** — log `provenance` so we can audit later.

## 9. Cross-session continuity

On session start:

1. Read `working.md` — restore last scratchpad.
2. Read top-N `preferences` for `agent=ash-sonic` — load rules into head.
3. Read top-N unresolved `open-questions.md` lines.
4. Background: warm the embedding cache for recent episodes (background,
   not blocking).

On session end (or on `compress` boundary):

1. Flush working state to `working.md`.
2. Write any open questions to `open-questions.md`.
3. Persist episodes + outcomes to `memory`.

## 10. Multi-agent coordination

The same `.ash/memory/` is shared across agents but **provenance is
per-agent**. Edits by `planner` get `agent=planner`, edits by `codex`
get `agent=codex`. The graph lets us ask: "what does the user think
vs. what does the planner think?" — and resolve conflicts by evidence
weight, not recency.

## 11. Refusal / safety

- Memory writes that include secrets (e.g. tokens in logs) are
  redacted by the writer; secrets never enter any table.
- Refusing destructive actions (`rm -rf`, `git push --force`) writes
  the refusal as a `decision` node with `kind=safety-refusal` so we
  audit it later.
- The agent never reads `agent.sqlite` rows from an agent other than
  itself unless the human explicitly asks.

## 12. Open questions (now in `open-questions.md`)

- Embedding model: default `all-MiniLM-L6-v2` (384d) for speed, or
  `bge-large` (1024d) for accuracy? Default to MiniLM until proven wrong.
- Episodic TTL: prune episodes older than 90 days by default, or never?
- Should vector + graph be merged into one SQLite (one file) for
  atomic backups, or kept separate for cleaner gitignore? Default:
  separate (clearer ownership).
- Should `procedural` `rulings` be revised upward (confidence * 1.05)
  each time they fire successfully, or held static until contradicted?
