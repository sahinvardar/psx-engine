#!/usr/bin/env bash

set -u
set -o pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
duckstation="${DUCKSTATION:-/Applications/DuckStation.app/Contents/MacOS/DuckStation}"
executable="$project_dir/build/hello_cube.exe"
emulator_pid=""

if [[ ! -x "$duckstation" ]]; then
	echo "DuckStation not found at: $duckstation" >&2
	echo "Set DUCKSTATION to the emulator executable path and try again." >&2
	exit 1
fi

source_hash() {
	{
		find "$project_dir/src" -type f -exec shasum {} +
		shasum \
			"$project_dir/CMakeLists.txt" \
			"$project_dir/CMakePresets.json"
	} | sort | shasum | awk '{ print $1 }'
}

stop_emulator() {
	if [[ -n "$emulator_pid" ]] && kill -0 "$emulator_pid" 2>/dev/null; then
		kill -KILL "$emulator_pid"
		wait "$emulator_pid" 2>/dev/null || true
	fi
	emulator_pid=""
}

start_emulator() {
	stop_emulator
	echo "Starting DuckStation with $executable"
	"$duckstation" -batch -fastboot -nofullscreen -- "$executable" &
	emulator_pid=$!
}

build_and_reload() {
	echo "Building hello_cube.exe..."
	if make --directory "$project_dir" --no-print-directory exe; then
		start_emulator
		echo "Watching for source changes. Press Ctrl-C to stop."
	else
		echo "Build failed; DuckStation will keep running the last successful build." >&2
	fi
}

trap stop_emulator EXIT INT TERM

last_hash="$(source_hash)"
build_and_reload

while true; do
	sleep 0.5
	current_hash="$(source_hash)"

	if [[ "$current_hash" != "$last_hash" ]]; then
		last_hash="$current_hash"
		build_and_reload
	fi
done
