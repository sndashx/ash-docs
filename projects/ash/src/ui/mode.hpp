#pragma once
/// Phase 11: thin alias header that re-exports the mode stack symbols.
///
/// Legacy code that does `#include "ui/mode.hpp"` keeps working; the
/// canonical home of the types is `ui/mode_stack.hpp`.
#include "ui/mode_stack.hpp"