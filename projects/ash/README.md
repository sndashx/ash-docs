# ASH — Ash of the Shattered Covenant

A hand-authored ASCII RPG written in C++20. Built around a truecolor
terminal renderer, an REXPaint (.xp) content pipeline, an entt-based
world model, and a hand-tuned dialogue + quest scripting system.

## Building

```bash
cmake -B build
cmake --build build -j
```

## Running

```bash
./build/ash
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

## Plan

The full design plan lives at the worktree root
(`00-conventions.txt` through `28-phase-23-release.txt`).
See `docs/architecture.md` for a summary of the 9 system pillars.
