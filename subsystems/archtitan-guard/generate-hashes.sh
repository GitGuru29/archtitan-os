#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
OUT="${SCRIPT_DIR}/hashes.h"

FILES=(
  "airootfs/usr/lib/archtitan/archtitan-guard"
  "airootfs/etc/pacman.d/hooks/archtitan-session-guard.hook"
  "airootfs/etc/systemd/system/archtitan-immutable-guard.service"
)

{
  echo "#ifndef ARCHTITAN_GUARD_HASHES_H"
  echo "#define ARCHTITAN_GUARD_HASHES_H"
} > "$OUT"

for f in "${FILES[@]}"; do
  FULL="${REPO_ROOT}/${f}"
  if [[ -f "$FULL" ]]; then
    H=$(sha256sum "$FULL" | awk '{print $1}')
  else
    H="MISSING"
  fi
  NAME=$(basename "$f" | tr '.' '_' | tr '-' '_')
  echo "#define HASH_${NAME} \"${H}\"" >> "$OUT"
done

echo "#endif" >> "$OUT"
