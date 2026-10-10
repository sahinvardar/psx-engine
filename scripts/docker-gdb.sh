#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
emulator_log="$(mktemp -t psx-duckstation-debug.XXXXXX)"
emulator_launcher_pid=""

cleanup() {
	if [[ -n "$emulator_launcher_pid" ]] && kill -0 "$emulator_launcher_pid" 2>/dev/null; then
		kill "$emulator_launcher_pid"
		wait "$emulator_launcher_pid" 2>/dev/null || true
	fi
	rm -f "$emulator_log"
}

trap cleanup EXIT INT TERM

"$project_dir/scripts/debug-emulator.sh" >"$emulator_log" 2>&1 &
emulator_launcher_pid=$!

for ((attempt = 0; attempt < 200; attempt++)); do
	if grep -q "DuckStation GDB server ready" "$emulator_log"; then
		break
	fi

	if ! kill -0 "$emulator_launcher_pid" 2>/dev/null; then
		wait "$emulator_launcher_pid" || true
		cat "$emulator_log" >&2
		exit 1
	fi

	sleep 0.1
done

if ! grep -q "DuckStation GDB server ready" "$emulator_log"; then
	cat "$emulator_log" >&2
	echo "Timed out while starting DuckStation for debugging." >&2
	exit 1
fi

docker run \
	--rm \
	--interactive \
	--platform linux/amd64 \
	--volume "$project_dir:/workspace" \
	--workdir /workspace \
	psx-hello-toolchain:0.24 \
	/usr/bin/gdb-multiarch \
	--interpreter=mi
