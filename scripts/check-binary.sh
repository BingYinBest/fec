#!/usr/bin/env bash
# Print ELF attributes of the built binary; used by build.sh.
set -euo pipefail

BIN="${1:?usage: check-binary.sh <binary>}"

if [ ! -x "$BIN" ]; then
  echo "[check] binary not found: $BIN" >&2
  exit 1
fi

if command -v readelf >/dev/null 2>&1; then
  readelf -h "$BIN" | grep -E "Class|Machine" || true
  if readelf -d "$BIN" 2>/dev/null | grep -q NEEDED; then
    echo "[check] WARNING: dynamically linked:"
    readelf -d "$BIN" | grep NEEDED
  else
    echo "[check] fully static (no dynamic dependencies)"
  fi
fi
