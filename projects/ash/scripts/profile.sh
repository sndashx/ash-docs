#!/usr/bin/env bash
# Tracy capture wrapper.
set -euo pipefail
cd "$(dirname "$0")/.."
exec tracy-capture -o trace.tracy -f ./build/ash "$@"
