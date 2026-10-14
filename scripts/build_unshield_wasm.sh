#!/bin/sh
# Builds unshield (MIT, github.com/twogood/unshield, pinned 0d1e7cb in
# research/toolchains/unshield) as an ES module + WebAssembly for the browser's
# installer import. Output: build/unshield/unshield.{js,wasm}.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
src="$root/research/toolchains/unshield"
emcc="$root/research/toolchains/emsdk/upstream/emscripten/emcc"
out="$root/build/unshield"
mkdir -p "$out"
[ "$(git -C "$src" rev-parse --short HEAD)" = 0d1e7cb ] || { echo "unshield is not at the pinned commit 0d1e7cb" >&2; exit 1; }
"$emcc" -O2 -sUSE_ZLIB=1 \
    -I "$root/port/unshield" -I "$src/lib" -I "$src/lib/md5" \
    -DHAVE_CONFIG_H -include "$root/port/unshield/lib/unshield_config.h" \
    "$src/src/unshield.c" "$src"/lib/*.c "$src/lib/md5/md5c.c" \
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createUnshield -sINVOKE_RUN=0 -sEXIT_RUNTIME=0 \
    -sEXPORTED_RUNTIME_METHODS=FS,callMain -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=2GB -sFORCE_FILESYSTEM=1 \
    -sENVIRONMENT=web,worker \
    -o "$out/unshield.js"
cp "$src/LICENSE" "$out/unshield-LICENSE.txt"
echo "$out/unshield.js"
