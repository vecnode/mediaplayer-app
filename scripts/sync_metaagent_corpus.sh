#!/usr/bin/env bash
# Sync metaagent media corpus sources into this app (single source of truth: metaagent repo).
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_METAAGENT="/c/Users/luisarandas/Documents/Unreal Projects/character2/Plugins/metaagent-ue5-plugin/metaagent"
METAAGENT_ROOT="${1:-${METAAGENT_ROOT:-$DEFAULT_METAAGENT}}"

SRC="${METAAGENT_ROOT}/src/media"
DEST="${ROOT_DIR}/src/metaagent/media"

mkdir -p "${DEST}"
mkdir -p "${ROOT_DIR}/src/metaagent/core"
cp -f "${SRC}/corpus.hpp" "${DEST}/"
cp -f "${SRC}/corpus.cpp" "${DEST}/"
cp -f "${METAAGENT_ROOT}/src/export.hpp" "${ROOT_DIR}/src/metaagent/export.hpp"
cp -f "${METAAGENT_ROOT}/src/core/types.hpp" "${ROOT_DIR}/src/metaagent/core/types.hpp"

echo "sync_metaagent_corpus: updated ${DEST}"
