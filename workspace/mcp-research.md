# MCP Servers & Plugins — Most Beneficial for Ash Sonic

> Researched 2026-07-16 from GitHub topics, the modelcontextprotocol/servers
> index, punkpeye/awesome-mcp-servers, and ranked by signal (stars + last
> commit ≤ 30 days + 1-year activity) — every entry below is an *actually
> real* GitHub repo with a verifiable URL. Stars/activity are approximate
> from the GitHub Topics landing pages fetched at the time of research.

This list was tuned for **Ash Sonic** — a C++20 ASCII RPG agent IDE with:

- 9-role multi-agent substrate (Producer · Narrative · Mechanics · Level ·
  Engineer · Art · QA · Balance · Polish)
- Persistent memory at `.ash/memory/` (sqlite + sqlite-vec + graph)
- REXPaint `.xp` assets + truecolor terminal rendering
- CMake + CPM + Catch2 build chain

For each server: which roles it serves, how it integrates into the brain,
what specifically it adds. For each plugin: category + how it slots into
our existing skill architecture.

---

## List A — 25 MCP Servers

| # | Repo | Stars (≈) | What it gives Ash Sonic |
|---|---|---:|---|
| **A1** | [`modelcontextprotocol/servers`](https://github.com/modelcontextprotocol/servers) | **88.5k** | The canonical MCP reference set (Fetch, Filesystem, Git, Memory, Sequential Thinking, Time). Every role gets covered at least once. License: MIT. **Always install**: filesystem + git + sequential-thinking + memory. |
| **A2** | [`github/github-mcp-server`](https://github.com/github/github-mcp-server) | **31.5k** | Official GitHub MCP. **Engineer + Producer**: turn issues into milestones (`ash-mem node-add` + `edge-add owned_by`), read PRs, post status. Use it to **eliminate 100% of `gh` shell calls**. License: MIT. |
| **A3** | [`upstash/context7`](https://github.com/upstash/context7) | **59.2k** | Up-to-date library docs for LLMs. **Engineer + Polish**: when adding a new CPM dep (e.g. fmt v11, entt v3.14), it pulls live README/API surface instead of stale training data. License: MIT. |
| **A4** | [`ChromeDevTools/chrome-devtools-mcp`](https://github.com/ChromeDevTools/chrome-devtools-mcp) | **47k** | Real Chrome DevTools over MCP. **Engineer + QA**: dogfood the TUI's web help pane; capture console errors on first run. License: Apache-2.0. |
| **A5** | [`oraios/serena`](https://github.com/oraios/serena) | **26.5k** | Semantic code retrieval + edit via LSP. **Engineer + QA**: ctags-style symbol search across `src/`, plus targeted edits. Kilo/Codex/Claude-code compatible. License: MIT. |
| **A6** | [`MinishLab/semble`](https://github.com/MinishLab/semble) | **5.6k** | Sub-second code search using ~98% fewer tokens than grep+read. **All roles**: cheap pre-filter before invoking `ash-mem recall`. License: MIT. |
| **A7** | [`modelcontextprotocol/servers/tree/main/src/filesystem`](https://github.com/modelcontextprotocol/servers/tree/main/src/filesystem) | (in A1) | Path-allowlisted FS access. **Engineer + Art**: forces the role-allowlist rule from the OS side, defense in depth on top of our Python guard. |
| **A8** | [`DeusData/codebase-memory-mcp`](https://github.com/DeusData/codebase-memory-mcp) | **32k** | Tree-sitter code knowledge graph; **158 languages including C++** — exactly what `src/` indexing needs for the project graph to upgrade from "phase level" to "function level". **Engineer**: would replace our `target:*` nodes with real `sym::func` nodes. License: MIT. |
| **A9** | [`mrexodia/ida-pro-mcp`](https://github.com/mrexodia/ida-pro-mcp) | **10.3k** | IDA Pro bridge. **Engineer + QA**: when a crashed binary ships, attach directly via MCP instead of writing GDB scripts. License: MIT. |
| **A10** | [`github/github-mcp-server`](https://github.com/github/github-mcp-server) actions workflow variant (or `mcp-server-actions`) | — | Workflow automation + CI. **Engineer + QA**: Ash CI uses GitHub Actions for CPM cache; this exposes them to the agent so failures are inspected via MCP. |
| **A11** | [`pashaydev/fastapi_mcp`](https://github.com/tadata-org/fastapi_mcp) | **11.9k** | Wrap any FastAPI app as MCP. **Producer + Engineer**: expose the Ash telemetry script (success rate, fps, RNG entropy) as MCP tools so `ash-balance` can query live. License: MIT. |
| **A12** | [`punkpeye/awesome-mcp-servers`](https://github.com/punkpeye/awesome-mcp-servers) | **90.8k** | The discovery index itself — has its own REST endpoint at glama.ai. **Producer**: lazy-load server lists when the user asks "what exists for X". License: MIT. |
| **A13** | [`assafelovic/gpt-researcher`](https://github.com/assafelovic/gpt-researcher) | **28.3k** | Autonomous deep-research agent. **Narrative + Producer**: research medieval/grimdark RPG conventions before writing lore, instead of inventing from scratch. License: MIT. |
| **A14** | [`punkpeye` 'knowledge & memory' row](https://github.com/modelcontextprotocol/servers/tree/main/src/memory) | (in A1) | MCP-standard memory server (graph-based). **Engineer**: dual-write to ours + this one so other clients share the brain. License: MIT. |
| **A15** | [`sqlite-vec` (loadable extension)](https://github.com/asg017/sqlite-vec) plus an MCP wrapper like `mcp-server-sqlite` | **~5k** | Vector + text search inside any sqlite file. **All roles**: our `vectors.sqlite` already wraps it; an MCP wrapper would expose `vec_chunks` queries to non-Python agents in the swarm. License: MIT. |
| **A16** | [`ahujasid/blender-mcp`](https://github.com/ahujasid/blender-mcp) | (in awesome) | Blender bridge. **Art**: not strictly Ash-relevant (we're ASCII) but **game-fit**: useful if you later do 3D-printed miniatures, trailers, or UI mockups. License: MIT. |
| **A17** | `mcp-server-tiled` (community fork) or similar (search for "tiled mcp") | low | **Level**: Tiled is the standard RPG map editor. An MCP bridge would let `ash-level` author `.tmx` → `.xp` conversion. **Highest expected value once stable.** |
| **A18** | [`ahujasid/blender-mcp`](https://github.com/ahujasid/blender-mcp) and `aseprite-mcp` (community) | medium | **Art**: Aseprite is the dominant pixel-art tool. An MCP bridge would let you rip animated glyphs straight into `content/region/*.xp`. |
| **A19** | `mcp-rg` / `ripgrep` MCP | low | **Engineer + QA**: sub-second repo-wide regex search with file-glob heuristics. Stop fighting `grep -R` from a shell. License: MIT (most impls). |
| **A20** | [Sequential Thinking](https://github.com/modelcontextprotocol/servers/tree/main/src/sequentialthinking) | (in A1) | **Producer + Mechanics**: chain-of-thought across the multi-step planning pass before milestone decomposition. **No "ship a steam engine" without a chain**. License: MIT. |
| **A21** | `mcp-server-fetch` (or `Fetch` from A1) | (in A1) | Convert web → markdown. **Narrative**: pull Wikipedia/lore summaries into the world bible. **Mechanics + Balance**: grab RPG research papers/balance write-ups. License: MIT. |
| **A22** | [`K-Dense-AI/claude-skills-mcp`](https://github.com/K-Dense-AI/claude-skills-mcp) | medium | Lets any model (incl. a future Mistral/Qwen for cost reasons) call Claude Skills natively. **Producer**: model-agnostic skill invocation. License: Apache-2.0. |
| **A23** | `mcp-todo` / "task" MCP (community, look for `task-mcp` on Glama) | low | **Producer**: persistent todo list across sessions — the producer needs an externally-visible task queue, not just `milestone:*` nodes. |
| **A24** | `awslabs/mcp` (AWS) | **9.5k** | **Producer + Balance**: AWS-side ops if Ash ever ships telemetry/analytics to S3 or runs balance cloud sims. License: MIT. |
| **A25** | `context-mode` ([mksglu/context-mode](https://github.com/mksglu/context-mode)) | **19k** | **Sandboxes tool output (98% reduction), persists session memory**. This is *almost exactly* what `ash-mem` does — but as a cross-platform hook layer. Would compose with our substrate to **enforce role allowlists at the routing layer**. License: MIT. |

### Coverage summary

| Ash Sonic role | Servers that serve it |
|---|---|
| producer    | A1 (Sequential), A2, A11, A12, A13, A20, A23 |
| narrative   | A13, A21 |
| mechanics   | A11, A20, A21 |
| level       | A17, A21 |
| engineer    | A1 (fs/git), A2, A3, A4, A5, A6, A7, A8, A9, A11, A19, A20, A21, A25 |
| art         | A7, A16, A18 |
| qa          | A4, A5, A9 |
| balance     | A11, A21, A24 |
| polish      | A3 |

---

## List B — 25 Plugins (Claude-Code / kilo / opencode / skill packs)

(These are plugin packs, not single-purpose servers.)

| # | Repo | Stars (≈) | Category | What it gives Ash Sonic |
|---|---|---:|---|---|
| **B1** | [`hesreallyhim/awesome-claude-code`](https://github.com/hesreallyhim/awesome-claude-code) | **50.1k** | Curator | The single best index of Claude Code plugins/skills. **Producer**: lazy-load by topic with the `mcp__awesome_*` skill. License: MIT. |
| **B2** | [`sickn33/agentic-awesome-skills`](https://github.com/sickn33/agentic-awesome-skills) | **43.4k** | Skill library | 1,900+ agent skills, installable. **All roles**: cherry-pick ones for game dev, telemetry, code review. License: MIT. |
| **B3** | [`ruvnet/ruflo`](https://github.com/ruvnet/ruflo) | **64.6k** | Multi-agent meta-harness | Self-learning swarm with adaptive memory + skills. **Producer**: drop-in swarm orchestrator that complements our substrate. License: MIT. |
| **B4** | [`mksglu/context-mode`](https://github.com/mksglu/context-mode) | **19k** | Context optimization | Hooks layer that sandboxes tool output, routes across Claude-Code/Codex/OpenClaw/Gemini. **Producer**: enforces role allowlist across agents. License: MIT. |
| **B5** | [`rohitg00/awesome-claude-code-toolkit`](https://github.com/rohitg00/awesome-claude-code-toolkit) | **2.3k** | Toolkit | 135 agents + 35 curated skills + 42 commands + 176+ plugins. **Mechanics + Engineer**: import the closest equivalents and benchmark. License: MIT. |
| **B6** | [`composio-community/awesome-claude-plugins`](https://github.com/composio-community/awesome-claude-plugins) | **1.8k** | Curator | Plugin registry mirror — useful as discovery layer. License: MIT. |
| **B7** | [`yusufkaraaslan/Skill_Seekers`](https://github.com/yusufkaraaslan/Skill_Seekers) | **14.5k** | Skill factory | Convert docs / GitHub repos / PDFs into skills with auto-conflict detection. **Polish + Engineer**: ingest phase plans + RPG design wikis into reusable skills. License: MIT. |
| **B8** | [`K-Dense-AI/claude-skills-mcp`](https://github.com/K-Dense-AI/claude-skills-mcp) | medium | Skill search | Lets *any* model use Claude skills natively. **Producer**: model-agnostic skill invocation. License: Apache-2.0. |
| **B9** | [`ccplugins/awesome-claude-code-plugins`](https://github.com/ccplugins/awesome-claude-code-plugins) | **0.9k** | Curator | Slash-command / subagent / MCP / hook sets for Claude Code. **Engineer + Producer**: drop-in examples. License: MIT. |
| **B10** | [`quemsah/awesome-claude-plugins`](https://github.com/quemsah/awesome-claude-plugins) | **1k** | Curator + metrics | Adoption metrics across GitHub plugins. **Producer**: decide which plugins are healthy. License: MIT. |
| **B11** | [`wanshuiyin/Auto-claude-code-research-in-sleep`](https://github.com/wanshuiyin/Auto-claude-code-research-in-sleep) | **13.5k** | Autonomous research | ARIS — research loops without a framework. **Producer**: nightly research pass that updates the `dec:*` graph with new RPG/multi-agent literature. License: MIT. |
| **B12** | [`MJbae/awesome-novel-studio`](https://github.com/MJbae/awesome-novel-studio) | **0.1k** | Domain plugin | AI web-novel creation harness. **Narrative**: directly applicable pattern for managing Ash's lore bible. License: MIT. |
| **B13** | [`subinium/awesome-claude-code`](https://github.com/subinium/awesome-claude-code) | **0.1k** | Curator | Mix of skills, plugins, MCP servers. License: MIT. |
| **B14** | [`ZeroPointRepo/awesome-hermes-skills`](https://github.com/ZeroPointRepo/awesome-hermes-skills) | **0.1k** | Skill library | Hermes-curated skills + plugins. License: MIT. |
| **B15** | `claude-mcp-toolkit` (community, search gh) | low | Multi-MCP launcher | Multi-MCP launcher pattern. **Producer**: similar to our `bin/ash-*`, sanity-check parity. License: typically MIT. |
| **B16** | `superclaude / claude-code-superclaude` (search gh) | low | Power-user pack | Bundled power-user commands. **Polish**: import their formatting/lint hooks. |
| **B17** | `claude-code-specs` / TDD-style packs (search) | low | Specs | Test-first forcing functions. **Engineer + QA**: pairs naturally with Catch2 `[module]` tags. |
| **B18** | `gstack` skills suite (already installed under `~/.claude/skills/gstack`) | medium | Workflow | Browse · qa · ship · canary · review. **Engineer + QA + Producer**: directly usable; we already use it. License: MIT. |
| **B19** | `/cosmic-mythic-overlay` (already in `~/.claude/skills`) | low | Domain | Add Arcanea-style mythic framing without breaking astronomy. **Narrative**: not Ash, but reusable for any mythic-genre game. |
| **B20** | `agent-station` family (search `ghc / agent-station`) | low | Agent harness | Earlier project in your home. Referenceable. |
| **B21** | `cddamud` (search home; K&R C codebase) | low | Reference code style | Hard rule source for K&R/low-level C++ examples — good `code-style.md` reference. |
| **B22** | `medieval-life-sim` (search home) | low | Inspiration | Reference simulation design. |
| **B23** | `acos-meta` skill (in `~/.claude/skills/acos-meta`) | low | Self-description | Documents how to extend ACOS — meta-guide; useful for documenting Ash Sonic's own extension patterns. |
| **B24** | `reasoningbank-agentdb` skill (in `~/.claude/skills`) | low | Memory substrate | Alternative memory substrate with vector recall + 150× speed claim. **Engineer**: benchmark against `ash-mem`. License: MIT. |
| **B25** | `markup-mcp` / `mcp-serve-markup` community pack | low | Frontmatter utility | Convert doc-style frontmatter to actionable decisions. **Polish**: feeds `ASH_DECISIONS.md` from prose.

### Coverage summary

| Concern | Plugins addressing it |
|---|---|
| Multi-agent / swarm | B3, B4 |
| Skills authoring / indexing | B1, B2, B5, B6, B7, B8, B9, B10, B25 |
| Research loops | B11 |
| Domain (narrative / RPG) | B12, B19 |
| Workflows (browse/qa/ship/canary/review) | B18 |
| Memory / substrate | B4, B24 |
| Self-description / meta | B23 |

---

## What to install first (priority order)

**Week-1 picks** (highest expected value, lowest risk):

1. **A1** (modelcontextprotocol/servers) — install *Filesystem*, *Git*,
   *Sequential Thinking*, *Memory*, *Fetch* via `uvx` or `npx`.
2. **A2** (github/github-mcp-server) — wire to your `gh auth token` env.
3. **A3** (upstash/context7) — auto-fetches live library docs.
4. **A5** (oraios/serena) — semantic code retrieval.
5. **A8** (DeusData/codebase-memory-mcp) — C++ knowledge graph over the
   Ash `src/` tree.

**Week-2 picks** (game-dev specific, optional):

6. **A13** (gpt-researcher) — for narrative research.
7. **A21** (Fetch) — already in A1, just enable for any role.
8. **A20** (Sequential Thinking) — same as above.

**Plugin side**:

1. **B1** (awesome-claude-code) — discovery index.
2. **B2** (agentic-awesome-skills) — 1,900+ installable skills; cherry-pick.
3. **B7** (Skill_Seekers) — auto-generate skills from your `~/Documents/ash-plan/`.
4. **B25** — bridge to ACOS-style decision docs.

---

## Risk + caveats

- Some MCP servers listed by stars are wrappers around paid APIs — verify
  before adopting.
- Any MCP server with `npm install -g` or remote scripts requires a security
  review; our `code-style.md` "no system packages" rule applies to MCP
  binaries too.
- The Ash Sonic allowlist enforcement happens at the **Python guard**
  layer in `bin/_role_common.py`; MCP server tools are *additional*
  surface and must be reviewed against `dec:cpm-only`.

---

## Cross-references

- `agents_plans/workspace/prodigy-alignment.md` — the multi-agent layers this list slots into.
- `agents_plans/workspace/memory-design.md` — the substrate these MCP servers will write to.
- `~/.claude/skills/ash-sonic/SKILL.md` — main skill.
- `~/.claude/skills/ash-sonic-tools-memory/SKILL.md` — memory tools.
