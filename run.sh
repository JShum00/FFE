#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="$SCRIPT_DIR/log.log"

if command -v stdbuf >/dev/null 2>&1; then
    LAUNCH_PREFIX=(stdbuf -oL -eL)
else
    LAUNCH_PREFIX=()
fi

: > "$LOG_FILE"

cd "$SCRIPT_DIR/build"

echo "Launching Frontline Forge Engine..." | tee -a "$LOG_FILE"
"${LAUNCH_PREFIX[@]}" ./RBDoom3BFG 2>&1 | tee -a "$LOG_FILE"
exit "${PIPESTATUS[0]}"
