#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cache_dir="$project_dir/.vscode/psn00bsdk"
include_dir="$cache_dir/include"
source_dir="$cache_dir/source"
sdk_commit="06e65bea3a778b2dae5af77a7935ae3868ddd4d3"

make --directory "$project_dir" --no-print-directory toolchain

rm -rf "$include_dir"
mkdir -p "$include_dir"

docker run --rm --platform linux/amd64 \
	--user "$(id -u):$(id -g)" \
	--volume "$project_dir:/workspace" \
	psx-hello-toolchain:0.24 \
	sh -c '
		mkdir -p \
			/workspace/.vscode/psn00bsdk/include/gcc \
			/workspace/.vscode/psn00bsdk/include/target
		cp -R \
			/opt/psn00bsdk/include/libpsn00b \
			/workspace/.vscode/psn00bsdk/include/
		cp -R \
			/opt/psn00bsdk/lib/gcc/mipsel-none-elf/12.3.0/include/. \
			/workspace/.vscode/psn00bsdk/include/gcc/
		cp -R \
			/opt/psn00bsdk/mipsel-none-elf/include/. \
			/workspace/.vscode/psn00bsdk/include/target/
	'

if [[ ! -d "$source_dir/.git" ]] \
	|| [[ "$(git -C "$source_dir" rev-parse HEAD 2>/dev/null || true)" != "$sdk_commit" ]]; then
	rm -rf "$source_dir"
	git clone --quiet --depth 1 --branch v0.24 \
		https://github.com/Lameguy64/PSn00bSDK.git \
		"$source_dir"
fi

actual_commit="$(git -C "$source_dir" rev-parse HEAD)"
if [[ "$actual_commit" != "$sdk_commit" ]]; then
	echo "Expected PSn00bSDK commit $sdk_commit, found $actual_commit" >&2
	exit 1
fi

echo "PSn00bSDK IntelliSense files are ready."
echo "In VS Code, run 'C/C++: Reset IntelliSense Database' if symbols do not appear."
