#!/bin/bash
# =========================================================================
# VanguardFPS - Local Co-op Multiplayer Launch Script (Linux)
# Launches two instances of VanguardFPS for instant LAN/Local testing
# =========================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_PATH="$SCRIPT_DIR/build/VanguardFPS"

if [ ! -f "$BIN_PATH" ]; then
    echo "[ERROR] Could not find $BIN_PATH"
    echo "Please build the project first: cmake -B build && cmake --build build"
    exit 1
fi

echo "[VANGUARD CO-OP] Launching Instance 1 (Squad Leader)..."
"$BIN_PATH" &
PID1=$!

sleep 1

echo "[VANGUARD CO-OP] Launching Instance 2 (Squad Operative)..."
"$BIN_PATH" &
PID2=$!

echo "[VANGUARD CO-OP] Instances running with PIDs: $PID1, $PID2"
echo "In Instance 1, click 'HOST CO-OP SQUAD LOBBY'."
echo "In Instance 2, select the discovered lobby in the list and click 'QUICK JOIN'."

