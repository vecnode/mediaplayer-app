#!/usr/bin/env bash
# Copy MinGW64 runtime DLLs next to the app so it runs outside the MSYS2 shell.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${ROOT_DIR}/bin"
EXE="${BIN_DIR}/media-player-cpp.exe"

if [[ ! -f "${EXE}" ]]; then
	echo "copy_msys2_dlls: ${EXE} not found — build first (make Release)" >&2
	exit 1
fi

echo "copy_msys2_dlls: copying runtime DLLs into ${BIN_DIR}"

while IFS= read -r dll_path; do
	cp -f "${dll_path}" "${BIN_DIR}/"
done < <(ldd "${EXE}" | awk '/mingw64\/bin/ { print $3 }' | sort -u)

echo "copy_msys2_dlls: done"
