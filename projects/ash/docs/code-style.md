# Code Style

ASH follows a small set of enforceable rules. The `.clang-format` and
`.clang-tidy` files at the project root are authoritative for anything
they cover; this document summarizes the rest.

- **Indent**: 4 spaces. No tabs.
- **Braces**: Allman — opening brace on its own line for functions,
  classes, namespaces, control flow.
- **Line length**: soft 100 columns, hard 120.
- **Naming**:
  - `snake_case` for functions, variables, and file names.
  - `PascalCase` for types (classes, structs, enums, aliases).
  - `SCREAMING_SNAKE_CASE` for compile-time constants and macros.
  - `k` + `PascalCase` for non-constexpr static class constants
    (e.g. `kDefaultRadius`).
- **Header guards**: `#pragma once` on every header. No traditional
  `ASH_FOO_BAR_HPP` guards.
- **Includes**: prefer the project-relative form `"module/foo.hpp"`
  over `<ash/foo.hpp>`. Order: matching header first, then C++ stdlib,
  then third-party, each group sorted.
- **Namespaces**: nested module namespaces (`ash::render::light`).
  No `using namespace` in headers.
- **Comments**: doxygen-style `///` for public API; `//` for
  implementation notes.
