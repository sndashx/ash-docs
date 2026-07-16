# ASH — Ash of the Shattered Covenant

> *A hand-authored ASCII RPG.*

ASH is a single-player role-playing game rendered entirely in the terminal
with a truecolor ANSI engine, an REXPaint (`.xp`) content pipeline, and
a hand-tuned dialogue + quest scripting system. The whole thing compiles
to a static C++20 binary that you talk to with your keyboard.

This document covers **Phase 0** — the project skeleton, build harness,
and entry-point banner. Subsequent phases add the renderer, world model,
content loaders, gameplay systems, and UI screens.

## Description

ASH is built around nine architectural pillars (see `docs/architecture.md`
and `04-pillars.txt`):

1. A 9-attribute / 24-skill character system with derived stats.
2. Real-time-with-pause combat, A* pathfinding, line-of-sight, cover.
3. An REXPaint (`.xp`) content pipeline — `.xp` files are the canonical
   authoring format for all maps, with 9 layers per map and a JSON
   sidecar schema for items / NPCs / triggers.
4. A hand-rolled dialogue scripting language (`*.dlg`) and a stage-machine
   quest engine (`*.qst`).
5. Save/load with a versioned schema and migrations.
6. An in-game editor — the same tool that ships with the game.

Phase 0 delivers only the buildable skeleton and the truecolor entry
banner. There is no gameplay yet.

## Build

Requirements: a C++20 compiler (GCC ≥ 11 or Clang ≥ 14), CMake ≥ 3.21,
and an internet connection on first configure (CPM downloads dependencies).

```bash
cmake -B build -S .
cmake --build build -j
```

The first configure downloads `entt`, `spdlog`, `fmt`, `nlohmann/json`,
and (for tests) `Catch2 v3` via CPM and caches them locally. Subsequent
builds are offline.

Useful CMake options:

| Option | Default | Effect |
|--------|---------|--------|
| `ASH_BUILD_TESTS` | `ON` | Build the `ash_tests` target and register CTest |
| `ASH_ENABLE_ASAN` | `OFF` | Compile and link with `-fsanitize=address` |
| `ASH_ENABLE_UBSAN` | `OFF` | Compile and link with `-fsanitize=undefined` |
| `ASH_WARNINGS_AS_ERRORS` | `OFF` | Pass `-Werror` |
| `ASH_HOT_RELOAD_CONTENT` | `OFF` | Hot-reload `.xp` / `.json` files during dev |
| `ASH_PROFILE` | `OFF` | Enable the Tracy profiler |

## Run

```bash
./build/ash                  # print the magenta + cyan banner, exit 0
./build/ash --version        # print version line
./build/ash --help           # print usage
./build/ash --log-level debug  # raise log verbosity for one run
```

The log level can also be set via the `ASH_LOG_LEVEL` environment
variable: `trace | debug | info | warn | error | critical | off`.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

Phase 0 ships five tests: a smoke test plus four `ansi` escape-sequence
tests (`move_cursor`, `set_fg`, `set_bg`, `reset`).

## Screenshot

```
$ ./build/ash
\x1b[38;2;255;80;200mASH v0.0.1 (First Spark)\x1b[0m
\x1b[38;2;80;200;255mA hand-authored ASCII RPG\x1b[0m
```

(In a truecolor terminal the first line renders in magenta and the
second in cyan.)

## License

MIT — see `LICENSE`.