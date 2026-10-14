#!/bin/sh
# Reproducibly extract the 9.6F installer into game/ and write the hashed
# asset-profile manifest. Needs the local unshield build:
#   git clone https://github.com/twogood/unshield research/toolchains/unshield  (pinned 0d1e7cb)
#   cmake -S research/toolchains/unshield -B research/toolchains/unshield/build \
#         -DCMAKE_BUILD_TYPE=Release -DUSE_OUR_OWN_MD5=ON -DBUILD_STATIC=ON
#   cmake --build research/toolchains/unshield/build
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"
inst=downloads/GTA2INSTALLER.exe
[ "$(md5 -q "$inst")" = 764848b25aed87b69da472f18c19580d ] || { echo "installer is not pristine" >&2; exit 1; }
[ -z "$(ls -A game 2>/dev/null)" ] || { echo "game/ is not empty; refusing to overwrite" >&2; exit 1; }

work=research/extract
rm -rf "$work" && mkdir -p "$work/disk1" "$work/all"
python3 -I scripts/extract/carve_sfx_payload.py "$inst" \
    research/runtime-data/inventory/sfx-payload-records.json "$work/disk1"
research/toolchains/unshield/build/src/unshield -d "$work/all" x "$work/disk1/data1.hdr" >"$work/unshield-x.log"
cp -R "$work/all/App_Executables/." game/
mkdir -p game/player   # the installer creates this empty save directory
python3 -I scripts/extract/hash_manifest.py game research/runtime-data/inventory/gta2-96f-file-manifest.json
rm -rf "$work"
