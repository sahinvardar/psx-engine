#!/usr/bin/env bash

set -u
set -o pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
duckstation="${DUCKSTATION:-/Applications/DuckStation.app/Contents/MacOS/DuckStation}"
executable="$project_dir/build/psx-engine.exe"
gdb_port="${PSX_GDB_PORT:-2345}"
emulator_pid=""

if [[ ! -x "$duckstation" ]]; then
	echo "DuckStation not found at: $duckstation" >&2
	echo "Set DUCKSTATION to the emulator executable path and try again." >&2
	exit 1
fi

if [[ ! -f "$executable" ]]; then
	echo "PS-X EXE not found at: $executable" >&2
	exit 1
fi

if nc -z 127.0.0.1 "$gdb_port" 2>/dev/null; then
	echo "GDB port $gdb_port is already in use." >&2
	exit 1
fi

stop_emulator() {
	if [[ -n "$emulator_pid" ]] && kill -0 "$emulator_pid" 2>/dev/null; then
		kill -KILL "$emulator_pid"
		wait "$emulator_pid" 2>/dev/null || true
	fi
}

trap stop_emulator EXIT INT TERM

"$duckstation" -batch -fastboot -nofullscreen -- "$executable" &
emulator_pid=$!

for ((attempt = 0; attempt < 100; attempt++)); do
	if ! kill -0 "$emulator_pid" 2>/dev/null; then
		wait "$emulator_pid"
		exit $?
	fi

	if { exec 3<>"/dev/tcp/127.0.0.1/$gdb_port"; } 2>/dev/null; then
		exec 3>&- 3<&-
		echo "DuckStation GDB server ready on port $gdb_port"
		wait "$emulator_pid"
		exit $?
	fi

	sleep 0.1
done

echo "Timed out waiting for DuckStation's GDB server on port $gdb_port." >&2
echo "Enable it in Settings > Advanced > Debugging and use port $gdb_port." >&2
exit 1
