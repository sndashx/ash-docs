#!/usr/bin/env bash
# Launch wrapper for ASH.
set -euo pipefail
cd "$(dirname "$0")/.."
exec ./build/ash "$@"
