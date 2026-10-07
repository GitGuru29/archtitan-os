#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
AIROOT_LIB="${REPO_ROOT}/airootfs/usr/lib/archtitan"

cd "${SCRIPT_DIR}"
gcc -O2 -s -Wall -o archtitan-guard main.c

mkdir -p "${AIROOT_LIB}"
install -Dm755 archtitan-guard "${AIROOT_LIB}/archtitan-guard"
