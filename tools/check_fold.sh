#!/usr/bin/env bash
set -euo pipefail
bin="${1:?usage: check_fold.sh <binary>}"

if strings "$bin" | grep -q 'schema_version'; then
  echo "FAIL: JSON key names still present in the binary" >&2
  exit 1
fi
echo "OK: no JSON key names in the binary"
objdump -dC --no-show-raw-insn "$bin" | awk '/pv_fixed_leg/,/ret/'
